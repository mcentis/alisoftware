#include "CalRun.hh"

#include "ConfigFileReader.hh"

#include "fstream"
#include "sstream"
#include "iostream"
#include "stdlib.h"

CalRun::CalRun(const char* binFile, ConfigFileReader* Conf):
  BinaryData(binFile, Conf)
{
  for(int i = 0; i < nChannels; ++i)
    for(int j = 0; j < nParameters; ++j)
      parameters[i][j] = 0;

  for(int i = 0; i < nChannels; ++i)
    {
      calHistos[i] = NULL;
      calProfiles[i] = NULL;
    }

  ReadPedFile(conf->GetValue("pedNoiseFile").c_str()); // read pedestal file

  histoDir = outFile->mkdir("Histograms");
  profileDir = outFile->mkdir("Profiles");
  outFile->cd();

  iSample = 1;
  iStep = 1;
}

CalRun::~CalRun()
{
}

void CalRun::analyseRunHeader()
{
  int posPipe = runHeader.find('|');
  int posSemiCol = runHeader.find_first_of(';');
  posPipe += 1;

  nSamples = atoi(runHeader.substr(posPipe, posSemiCol - posPipe).c_str());

  posSemiCol += 1;
  int nextSemiCol = runHeader.find_first_of(';', posSemiCol);

  startCharge = atoi(runHeader.substr(posSemiCol, nextSemiCol - posSemiCol).c_str());

  posSemiCol = nextSemiCol + 1;
  nextSemiCol = runHeader.find_first_of(';', posSemiCol);

  endCharge = atoi(runHeader.substr(posSemiCol, nextSemiCol - posSemiCol).c_str());

  posSemiCol = runHeader.find_last_of(';');
  posSemiCol += 1;

  stepSize = atoi(runHeader.substr(posSemiCol, runHeader.size()).c_str());

  std::cout << "Start injected charge: " << startCharge << std::endl;
  std::cout << "End injected charge: " << endCharge << std::endl;
  std::cout << "Step injected charge: " << stepSize << std::endl;
  std::cout << "Number of samples: " << nSamples << std::endl;
  std::cout << "Expected events: " << (endCharge - startCharge) / stepSize * nSamples * 2 << std::endl; // the * 2 factor comes since both positive and negative signals are measured

  createHistos();

  return;
}

void CalRun::createHistos()
{
  char name[20];
  char title[100];

  int nSteps = (endCharge - startCharge) / stepSize * 2; // number of steps in the charge injection

  for(int i = 0; i < nChannels; ++i)
    {
      sprintf(name, "calHistCh_%d", i);
      sprintf(title, "Calibration channel %d;Injected charge [ALI e^{-}];Measured pulse height [ADC]", i);
      calHistos[i] = new TH2F(name, title, nSteps + 1, -endCharge - stepSize / 2., endCharge + stepSize / 2., 1024, -511.5, 511.5);
    }

  return;
}

void CalRun::doSpecificStuff()
{
  injCharge = iStep * stepSize;

  for(int i = 0; i < nChannels; ++i)
    {
      if(iSample % 2) // every event the polarity of the charge is inverted
      {
    	if(i % 2) calHistos[i]->Fill(injCharge, adcPH[i] - pedestals[i]);
    	else calHistos[i]->Fill(-injCharge, adcPH[i] - pedestals[i]); // channels with even channel number get inverted
      }
    else
      {
    	if(i % 2) calHistos[i]->Fill(-injCharge, adcPH[i] - pedestals[i]); // channels with odd channel number get inverted
    	else calHistos[i]->Fill(injCharge, adcPH[i] - pedestals[i]);
      }
    }

  if(iSample == nSamples * 2) // nSamples for each polarity
    {
      iSample = 0;
      iStep++;
    }

  ++iSample;

  return;
}

void CalRun::AnalyseData()
{
  char name[20];
  char title[100];

  for(int i = 0; i < nChannels; ++i) // generate the profiles
    {
      sprintf(name, "calProfileCh_%d", i);
      sprintf(title, "Calibration channel %d;Injected charge [ALI e^{-}];Measured pulse height [ADC]", i);
      calProfiles[i] = calHistos[i]->ProfileX(name);
      calProfiles[i]->SetTitle(title);
    }

  writeHistos();
  writeProfiles();

  return;
}

void CalRun::writeHistos()
{
  histoDir->cd();
  for(int i = 0; i < nChannels; ++i)
    calHistos[i]->Write();

  return;
}

void CalRun::writeProfiles()
{
  profileDir->cd();
  for(int i = 0; i < nChannels; ++i)
    calProfiles[i]->Write();

  return;
}

void CalRun::ReadPedFile(const char* pedFile)
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
