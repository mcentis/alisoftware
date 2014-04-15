#include "BinaryData.hh"

#include "iostream"
#include "time.h"
#include "stdlib.h"
#include "stdint.h"
#include "algorithm"

#include "TF1.h"
#include "TCanvas.h"
#include "TString.h"

BinaryData::BinaryData(const char* InFileName, ConfigFileReader* Conf)
{
  conf = Conf;

  // add check of the conf?

  inFileName = InFileName;
  outFilePath = conf->GetValue("outPath");
  runNumber = -1;
  sizeTime = atoi(conf->GetValue("timeSize").c_str());

  for(int i = 0; i < nChannels; ++i)
    adcPH[i] = -1;

  // ================ open the file stream ===============
  fileStr.open(inFileName.c_str(), std::ifstream::in);

  if(fileStr.is_open() == false)
    {
      std::cout << "Impossible to open the file " << inFileName << std::endl;
      exit(1);
    }

  // get run number from input file name
  ExtractRunNumber();

  // ============== open root file =======================
  char outFileName[400];
  sprintf(outFileName, "%s/run%06i.root", outFilePath.c_str(), runNumber);

  outFile = new TFile(outFileName, "RECREATE");

  // ============= define the tree ======================
  rawEvtTree = new TTree("rawEvtTree", "Raw data events");
  rawEvtTree->Branch("injCharge", &injCharge, "injCharge/F");
  rawEvtTree->Branch("delay", &delay, "delay/F");
  rawEvtTree->Branch("time", &time, "time/F");
  rawEvtTree->Branch("adcPH", adcPH, TString::Format("adcPH[%i]/i", nChannels)); // trick with the string to have the right array length
  rawEvtTree->Branch("temp", &temp, "temp/F");

  allEvents = new TH2I("allEvents", "Raw PH all channels;Channel;Pulse height [ADC]", 256, -0.5, 255.5, 1024, -0.5, 1023.5);
}

BinaryData::~BinaryData()
{
  fileStr.close();
  outFile->Close();
}

void BinaryData::ExtractRunNumber()
{
  unsigned int first = inFileName.find_last_of("run"); // position of the n of run
  unsigned int dot = inFileName.find_last_of(".");

  std::cout << "===================== Correct run number finding !!! ====================================" << std::endl;

  if(first >= inFileName.size() || dot >= inFileName.size() || first >= dot)
    {
      std::cout << "[BinaryData::ExtractRunNumber] Unable to determine the run number frome the input file name, run number set to -1" << std::endl;
      runNumber = -1;
      return;
    }

  first += 1; // exclude the n of run
  std::string strNumber = inFileName.substr(first, dot - first);
  runNumber = atoi(strNumber.c_str());

  // std::cout << "first: " << first << " dot: " << dot << "   " << strNumber << std::endl; // for testing only

  return;
}

void BinaryData::ReadFile()
{
  // =================================== file header =======================================
  time_t date;
  int32_t type;
  uint32_t headerLength;
  char* header;
  int version;

  fileStr.read((char*) &date, sizeTime);
  fileStr.read((char*) &type, sizeInt32);
  fileStr.read((char*) &headerLength, sizeInt32);

  header = new char[headerLength];
  fileStr.read(header, headerLength);

  runHeader = header;
  runType = type;

  if(header[0] != 'v' && header[0] != 'V')
    version = 0;
  else 
    version = atoi(&header[1]);

  std::cout << "====================== File header ====================" << std::endl;

  std::cout <<  "Time = " << ctime(&date);
  std::cout <<  "Type = " << type << " -> ";
  switch(type)
    {
    case 1:
      std::cout << "calibration run" << std::endl;
      break;
    case 2:
      std::cout << "laser sync. run" << std::endl;
      break;
    case 3:
      std::cout << "laser run" << std::endl;
      break;
    case 4:
      std::cout << "radioactive source run" << std::endl;
      break;
    case 5:
      std::cout << "pedestal run" << std::endl;
      break;
    default:
      std::cout << "unknown run type" << std::endl;
    }

  std::cout <<  "Header = " << header << std::endl;
  std::cout <<  "Version = " << version << std::endl;

  analyseRunHeader();

  // =============================== pedestals and noise =============================================
  double ped[nChannels];
  double noise[nChannels];

  fileStr.read((char*) &ped, sizeDouble * nChannels);
  fileStr.read((char*) &noise, sizeDouble * nChannels);

  // ============================= data blocks ===============================================================

  uint32_t dbHead = 0;
  unsigned int dbType;
  bool isUserDb;
  uint32_t dbSize;
  char* dataBlock;
  int evtCount = 0;

  double scanValue;
  uint32_t rawTime;
  uint16_t rawTemp;
  uint16_t evtHeader[32];
  uint16_t tempAdcPH[nChannels]; // temporary data storage to have conversion from uint16_t to UInt_t

  if(version == 0)
    {
      std::cout << "Version is 0 => Data are not going to be readen (so it is in the alibava code)" << std::endl;
      fileStr.close();
      return;
    }

  if(version == 1)
    {
      std::cout << "Version is 1 => The program has no implementation for these data, the alibava code contains one" << std::endl;
      fileStr.close();
      return;
    }

  do // loop to read the file to the end
    {
      try
	{
	  do // find the data block header
	    {
	      fileStr.read((char*) &dbHead, sizeUint32);
	      if(!fileStr.good())
		{
		  std::cout << "File stream not good while looking for data block header" << std::endl;
		  throw 1;
		}
	    } while((dbHead & 0xffff0000) != 0xcafe0000); // find the data block header
	}

      catch(int a)
	{
	  std::cout << "Exception catched: " << a << std::endl;
	  break;
	}

      dbType = dbHead & 0xfff;
      isUserDb = dbHead & 0x1000;

      fileStr.read((char*) &dbSize, sizeUint32);

      switch(dbType) // if the data block is not data is just readen and trown away
	{
	case NewFile:
	  dataBlock = new char[dbSize];
	  fileStr.read(dataBlock, dbSize);
	  delete dataBlock;
	  break;
	case StartOfRun:
	  dataBlock = new char[dbSize];
	  fileStr.read(dataBlock, dbSize);
	  delete dataBlock;
	  break;
	case Data:
	  if(isUserDb) // user data, structure not known
	    {
	      dataBlock = new char[dbSize];
	      fileStr.read(dataBlock, dbSize);
	      delete dataBlock;
	      break;
	    }

	  fileStr.read((char*) &scanValue, sizeDouble);
	  injCharge = (double) (int(scanValue) & 0xff);
	  injCharge *= 1024;
	  delay = (double) (int(scanValue) >> 16);

	  fileStr.read((char*) &rawTime, sizeUint32);
	  fileStr.read((char*) &rawTemp, sizeUint16);

	  time = convertTime(rawTime);
	  temp = convertTemp(rawTemp);

	  for(int ichip = 0; ichip < 2; ++ichip)
	    {
	      fileStr.read((char*) &evtHeader[ichip * 16], 16 * sizeUint16);
	      fileStr.read((char*) &tempAdcPH[ichip * (nChannels / 2)], (nChannels / 2) * sizeUint16);
	    }

	  for(int ichan = 0; ichan < nChannels; ++ichan)
	    {
	      adcPH[ichan] = tempAdcPH[ichan];
	      allEvents->Fill(ichan, adcPH[ichan]);
	    }

	  evtCount++;
	  rawEvtTree->Fill();
	  doSpecificStuff(); // to allow daughter classes to do stuff in this loop
	  break;
	case CheckPoint:
	  dataBlock = new char[dbSize];
	  fileStr.read(dataBlock, dbSize);
	  delete dataBlock;
	  break;
	case EndOfRun:
	  dataBlock = new char[dbSize];
	  fileStr.read(dataBlock, dbSize);
	  delete dataBlock;
	  break;
	default:
	  std::cout << "Data block type unknown" << std::endl;
	}
    } while(fileStr.good());

  std::cout << "==================================================" << std::endl;
  std::cout << "Total events = " << evtCount << std::endl;

  delete header;

  return;
}

void BinaryData::WriteEvts()
{
  outFile->cd();
  rawEvtTree->Write();
  allEvents->Write();

  return;
}

Float_t BinaryData::convertTime(uint32_t rawT)
{
  int16_t ipart = (rawT & 0xffff0000) >> 16;
  if(ipart < 0) ipart *= -1;
  uint16_t fpart = rawT & 0xffff;
  return 100 * (ipart + fpart / 65535.);
}

Float_t BinaryData::convertTemp(uint16_t rawT)
{
  return 0.12 * rawT - 39.8;
}

void BinaryData::readGoodChFile(const char* chFileName)
{
  goodChannels.clear();

  ifstream chStr;
  chStr.open(chFileName, std::ifstream::in);

  if(chStr.is_open() == false)
    {
      std::cout << "Impossible to open the file " << chFileName << "\nAll channels will be used" << std::endl;
      for(int i = 0; i < nChannels; i++) goodChannels.push_back(i);
      return;
    }

  std::string line;
  std::string::iterator newEnd; // end of the string after remove
  unsigned int sharpPos; // position of the #

  while(!chStr.eof())
    {
      line.clear();

      std::getline(chStr, line);

      if(line.length() == 0) continue; // empty line

      sharpPos = line.find('#');
      if(sharpPos == 0) continue; // full comment line, skip

      if(sharpPos < line.size()) // sharp found
      line.resize(sharpPos); // ignore what comes after the #

      // removes all the spaces from the string, remove does not change the length of the string, it is necessary to resize the string
      newEnd = std::remove(line.begin(), line.end(), ' ');
      if(newEnd == line.begin()) continue; // string of spaces and comments
      line.resize(newEnd - line.begin()); // resize the string to its new size

      // same treatment for the \t
      newEnd = std::remove(line.begin(), line.end(), '\t');
      if(newEnd == line.begin()) continue; // string of spaces, tabs and comments
      line.resize(newEnd - line.begin()); // resize the string to its new size

      goodChannels.push_back(atoi(line.c_str()));
    }

  chStr.close();

  return;
}
