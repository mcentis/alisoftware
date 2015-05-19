#include "DelRun.hh"

#include "ConfigFileReader.hh"

#include "fstream"
#include "sstream"
#include "iostream"
#include "stdlib.h"

DelRun::DelRun(const char* binFile, ConfigFileReader* Conf):
  BinaryData(binFile, Conf)
{
  for(int i = 0; i < nChannels; ++i)
    for(int pol = 0; pol < 2; ++pol)
    {
      delHistos[i][pol] = NULL;
      delProfiles[i][pol] = NULL;
    }

  ReadPedFile(conf->GetValue("pedNoiseFile").c_str()); // read pedestal file
  readGoodChFile(conf->GetValue("goodChFile").c_str()); // read good channels file

  histoDir[0] = outFile->mkdir("Histograms_Negative_charge");
  profileDir[0] = outFile->mkdir("Profiles_Negative_charge");

  histoDir[1] = outFile->mkdir("Histograms_Positive_charge");
  profileDir[1] = outFile->mkdir("Profiles_Positive_charge");
}

DelRun::~DelRun()
{
}

void DelRun::analyseRunHeader()
{
  int posPipe = runHeader.find('|');
  int posSemiCol = runHeader.find_first_of(';');
  posPipe += 1;

  nSamples = atoi(runHeader.substr(posPipe, posSemiCol - posPipe).c_str());

  posSemiCol += 1;
  int nextSemiCol = runHeader.find_first_of(';', posSemiCol);

  startDel = atoi(runHeader.substr(posSemiCol, nextSemiCol - posSemiCol).c_str());

  posSemiCol = nextSemiCol + 1;
  nextSemiCol = runHeader.find_first_of(';', posSemiCol);

  endDel = atoi(runHeader.substr(posSemiCol, nextSemiCol - posSemiCol).c_str());

  posSemiCol = nextSemiCol + 1;
  nextSemiCol = runHeader.find_first_of(';', posSemiCol);

  stepSize = atoi(runHeader.substr(posSemiCol, nextSemiCol - posSemiCol).c_str());

  std::cout << "Start delay: " << startDel << std::endl;
  std::cout << "End delay: " << endDel << std::endl;
  std::cout << "Step delay: " << stepSize << std::endl;
  std::cout << "Number of samples: " << nSamples << std::endl;
  std::cout << "Expected events: " << (endDel - startDel) / stepSize * nSamples * 2 << std::endl; // the * 2 factor comes since both positive and negative signals are measured

  createHistos();

  return;
}

void DelRun::createHistos()
{
  char name[20];
  char title[100];

  int nSteps = (endDel - startDel) / stepSize; // number of steps in the charge injection

  for(int i = 0; i < nChannels; ++i)
    {
      sprintf(name, "delHistCh_%d_neg", i);
      sprintf(title, "Delay scan channel %d, negative polarity;Delay [ns];Measured pulse height [ADC]", i);
      delHistos[i][0] = new TH2F(name, title, nSteps + 1, startDel - stepSize / 2., endDel + stepSize / 2., 1024, -511.5, 511.5);

      sprintf(name, "delHistCh_%d_pos", i);
      sprintf(title, "Delay scan channel %d, positive polarity;Delay [ns];Measured pulse height [ADC]", i);
      delHistos[i][1] = new TH2F(name, title, nSteps + 1, startDel - stepSize / 2., endDel + stepSize / 2., 1024, -511.5, 511.5);
    }

  delHistosGoodCh[0] = new TH2F("delHistGoodCh_neg", "Sum of delay scan good channels, negative polarity;Delay [ns];Measured pulse height [ADC]", nSteps + 1, startDel - stepSize / 2., endDel + stepSize / 2., 1024, -511.5, 511.5);
  delHistosGoodCh[1] = new TH2F("delHistGoodCh_pos", "Sum of delay scan good channels, positive polarity;Delay [ns];Measured pulse height [ADC]", nSteps + 1, startDel - stepSize / 2., endDel + stepSize / 2., 1024, -511.5, 511.5);

  return;
}

void DelRun::doSpecificStuff()
{
  for(int i = 0; i < nChannels; ++i)
    {
      if(iSample % 2) // every event the polarity of the charge is inverted
      {
    	if(i % 2) delHistos[i][0]->Fill(delay, adcPH[i] - pedestals[i]);
    	else delHistos[i][1]->Fill(delay, adcPH[i] - pedestals[i]); // channels with even channel number get positive polarity
      }
    else
      {
    	if(i % 2) delHistos[i][1]->Fill(delay, adcPH[i] - pedestals[i]); // channels with odd channel number get positive polarity
    	else delHistos[i][0]->Fill(delay, adcPH[i] - pedestals[i]);
      }
    }


  if(iSample == nSamples * 2) // nSamples for each polarity
    {
      std::cout << delay << std::endl;

      iSample = 0;
    }

  ++iSample;

  return;
}

void DelRun::AnalyseData()
{
  char name[20];
  char title[100];

  for(int i = 0; i < nChannels; ++i) // generate the profiles
    for(int pol = 0; pol < 2; ++pol)
      {
	if(pol == 0) // negative polarity
	  {
	    sprintf(name, "delProfileCh_%d_neg", i);
	    sprintf(title, "Delay scan channel %d, negative polarity;Delay [ns];Measured pulse height [ADC]", i);
	  }
	else
	  {
	    sprintf(name, "delProfileCh_%d_pos", i);
	    sprintf(title, "Delay scan channel %d, positive polarity;Delay [ns];Measured pulse height [ADC]", i);
	  }

      delProfiles[i][pol] = delHistos[i][pol]->ProfileX(name);
      delProfiles[i][pol]->SetTitle(title);
    }

  for(unsigned int i = 0; i < goodChannels.size(); ++i)
    for(int pol = 0; pol < 2; ++pol)
      delHistosGoodCh[pol]->Add(delHistos[goodChannels.at(i)][pol]);
  
  delProfileGoodCh[0] = delHistosGoodCh[0]->ProfileX("delProfileGoodCh_neg");
  delProfileGoodCh[0]->SetTitle("Delay scan good channels, negative polarity;Delay [ns];Measured pulse height [ADC]")

  delProfileGoodCh[1] = delHistosGoodCh[1]->ProfileX("delProfileGoodCh_pos");
  delProfileGoodCh[1]->SetTitle("Delay scan good channels, positive polarity;Delay [ns];Measured pulse height [ADC]")

  writeHistos();
  writeProfiles();

  return;
}

void DelRun::writeHistos()
{
  for(int pol = 0; pol < 2; ++pol)
    {
      histoDir[pol]->cd();
      for(int i = 0; i < nChannels; ++i)
	delHistos[i][pol]->Write();
    }

  outFile->cd();
  for(int pol = 0; pol < 2; ++pol)
    delHistGoodCh[pol]->Write();

  return;
}

void DelRun::writeProfiles()
{
  for(int pol = 0; pol < 2; ++pol)
    {
      profileDir[pol]->cd();
      for(int i = 0; i < nChannels; ++i)
	delProfiles[i][pol]->Write();
    }

  outFile->cd();
  for(int pol = 0; pol < 2; ++pol)
    delProfileGoodCh[pol]->Write();

  return;
}

void DelRun::ReadPedFile(const char* pedFile)
{
  ifstream pedStr;
  pedStr.open(pedFile, std::ifstream::in);

  if(pedStr.is_open() == false)
    {
      std::cout << "[CalRun::ReadPedFile] Impossile to open " << pedFile << " all pedestals and noises set to -1" << std::endl;
      for(int i = 0; i < nChannels; ++i)
	{
	  pedestals[i] = -1;
	  noise[i] = -1;
	}

      return;
    }

  std::string line;
  std::istringstream lineStr;

  int chNum;

  while(pedStr.good())
    {
      std::getline(pedStr, line);

      lineStr.clear();
      lineStr.str(line);

      lineStr >> chNum;
      lineStr >> pedestals[chNum] >> noise[chNum];
    }

  pedStr.close();

  return;
}
