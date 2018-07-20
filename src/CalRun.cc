#include "CalRun.hh"

#include "ConfigFileReader.hh"

#include "fstream"
#include "sstream"
#include "iostream"
#include "stdlib.h"

#include "TF1.h"
#include "TCanvas.h"

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
  readGoodChFile(conf->GetValue("goodChFile").c_str()); // read good channels file

  for(int iCh = 0; iCh < nChannels; ++iCh) isGoodCh[iCh] = false;
  for(unsigned int i = 0; i < goodChannels.size(); ++i) isGoodCh[goodChannels.at(i)] = true;

  startFit = atof(conf->GetValue("fitStart").c_str());
  endFit = atof(conf->GetValue("fitEnd").c_str());

  histoDir = outFile->mkdir("Histograms");
  profileDir = outFile->mkdir("Profiles");

  parCal = outFile->mkdir("Graphs_Parameters");

  outFile->cd();

  redChi2Cal = new TH1F("redChi2Cal", "Reduced #chi^{2} of the calibrations fit;#chi^{2} / NDF;Entries", 30, 0, 30);

  redChi2vsCh = new TGraph();
  redChi2vsCh->SetName("redChi2vsCh_posCal");
  redChi2vsCh->SetTitle("Reduced #chi^{2} of the calibrations fit for the positive charge");


  char name[50];
  char title[200];

  for(int i = 0; i < nParameters; ++i)
    {
      parVsCh[i] = new TGraph();
      sprintf(name, "par_%i_vsCh", i);
      sprintf(title, "Fit parameter %i vs channel", i);
      parVsCh[i]->SetName(name);
      parVsCh[i]->SetTitle(title);
    }

  iSample = 1;
  // iStep = 1;
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
  // charge = iStep * stepSize;

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

      std::cout << injCharge << std::endl;

      iSample = 0;
      // iStep++;
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

  fitCalibrations();

  writeHistos();
  writeProfiles();
  drawParGraphs();
  writeParGraphs();

  writeParList();

  return;
}

void CalRun::fitCalibrations()
{
  TF1* fitFunc;
  TCanvas* servCan = new TCanvas();

  bool linFit = false; // is the fitting done with a pol1
  if(strcmp(fitCal, "pol1") == 0) // if the strings are the same
    linFit = true;
  
  for(int iCh = 0; iCh < nChannels; iCh++)
    {
      fitFunc = new TF1("fitFunc", fitCal, startFit, endFit);
      fitFunc->SetLineColor(kRed);

      calProfiles[iCh]->Fit(fitFunc, "QR+"); // the + is to store both the fits in the profile

      fitFunc->GetParameters(parameters[iCh]);

      if(linFit) // if linear fit, save absolute gain value
	parameters[iCh][1] = fabs(parameters[iCh][1]);
      
      redChi2Cal->Fill(fitFunc->GetChisquare() / fitFunc->GetNDF());

      redChi2vsCh->SetPoint(iCh, iCh, fitFunc->GetChisquare() / fitFunc->GetNDF());

      delete fitFunc;
    }
  
  delete servCan;

  return;
}

void CalRun::writeHistos()
{
  histoDir->cd();
  for(int i = 0; i < nChannels; ++i)
    calHistos[i]->Write();

  outFile->cd();
  redChi2Cal->Write();

  return;
}

void CalRun::drawParGraphs()
{
  for(int iCh = 0; iCh < nChannels; iCh++)
    for(int iPar = 0; iPar < nParameters; ++iPar)
	parVsCh[iPar]->SetPoint(iCh, iCh, parameters[iCh][iPar]);


  TCanvas* srvCan = new TCanvas();
  srvCan->cd();

  for(int iPar = 0; iPar < nParameters; ++iPar)
    {
      parVsCh[iPar]->Draw("A*");
      parVsCh[iPar]->GetXaxis()->SetTitle("Channel");
      parVsCh[iPar]->GetYaxis()->SetTitle("Par. value");
    }

  delete srvCan;

  char name[50];
  char title[200];
  float min, max;

  for(int iPar = 0; iPar < nParameters; ++iPar)
    {
      sprintf(name, "distrPar_%i", iPar);
      sprintf(title, "Distribution of parameter %i", iPar);
      min = parVsCh[iPar]->GetYaxis()->GetXmin();
      max = parVsCh[iPar]->GetYaxis()->GetXmax();

      parDistr[iPar] = new TH1F(name, title, 200, min, max);

      sprintf(name, "distrPar_%i_goodCh", iPar);
      sprintf(title, "Distribution of parameter %i, good channels", iPar);
      parDistr_goodCh[iPar] = new TH1F(name, title, 200, min, max);
    }

  for(int iCh = 0; iCh < nChannels; iCh++)
    for(int iPar = 0; iPar < nParameters; ++iPar)
      {
	parDistr[iPar]->Fill(parameters[iCh][iPar]);

	if(isGoodCh[iCh])
	    parDistr_goodCh[iPar]->Fill(parameters[iCh][iPar]);
      }

  return;
}

void CalRun::writeParGraphs()
{
  outFile->cd();

  TCanvas* srvCan = new TCanvas();
  srvCan->cd();

  redChi2vsCh->Draw("A*");
  redChi2vsCh->GetXaxis()->SetTitle("Channel");
  redChi2vsCh->GetYaxis()->SetTitle("#chi^{2} / NDF");
  redChi2vsCh->Write();

  delete srvCan;

  parCal->cd();
  for(int iPar = 0; iPar < nParameters; ++iPar) parVsCh[iPar]->Write();
  for(int iPar = 0; iPar < nParameters; ++iPar) parDistr[iPar]->Write();
  for(int iPar = 0; iPar < nParameters; ++iPar) parDistr_goodCh[iPar]->Write();

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
  std::ifstream pedStr;
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

void CalRun::writeParList()
{
  char fileName[300];
  sprintf(fileName, "%s/CalPar%06d.list", outFilePath.c_str(), runNumber);
  std::ofstream parList;
  parList.open(fileName);

  for(int iCh = 0; iCh < nChannels; ++iCh)
    {
      parList << iCh;
      for(int iPar = 0; iPar < nParameters; ++iPar) parList <<'\t' << parameters[iCh][iPar];
      
      parList << '\n';
    }
  
  parList.close();

  return;
}
