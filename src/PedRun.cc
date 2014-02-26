#include "PedRun.hh"

#include "ConfigFileReader.hh"

#include "TF1.h"
#include "TCanvas.h"

#include "iostream"
#include "sstream"
#include "stdlib.h"

PedRun::PedRun(const char* binFile, ConfigFileReader* Conf):
  BinaryData(binFile, Conf)
{
  for(int i = 0; i < nChannels; ++i)
    {
      pedestals[i] = -1;
      noise[i] = -1;
    }

  pedDir = outFile->mkdir("Pedestals");
  pedDir->cd();

  char name[20];
  char title[100];

  for(int i = 0; i < nChannels; ++i)
    {
      sprintf(name, "pedCh_%d", i);
      sprintf(title, "Pedestal channel %d;Pulse height [ADC];Entries / channel", i);
      pedHist[i] = new TH1I(name, title, 1024, -0.5, 1023.5);
    }

  noiseDir = outFile->mkdir("Noise");
  noiseDir->cd();

  readGoodChFile(conf->GetValue("goodChFile").c_str());

  for(unsigned int i = 0; i < goodChannels.size(); ++i)
    {
      sprintf(name, "noiseCh_%d", goodChannels[i]);
      sprintf(title, "Noise channel %d;Pulse height [ADC];Entries / channel", goodChannels[i]);
      noiseHist.push_back(new TH1F(name, title, 100, -20, 20));
    }

  PedGraph = new TGraphErrors();
  PedGraph->SetName("PedGraph");
  PedGraph->SetTitle("Pedestals for all the channels");
  PedGraph->SetMarkerStyle(7);

  RawNoiseGraph = new TGraphErrors();
  RawNoiseGraph->SetName("RawNoiseGraph");
  RawNoiseGraph->SetTitle("Raw noise for all the channels");
  RawNoiseGraph->SetMarkerStyle(7);

  NoiseGraph = new TGraphErrors();
  NoiseGraph->SetName("NoiseGraph");
  NoiseGraph->SetTitle("Noise for the good channels");
  NoiseGraph->SetMarkerStyle(7);

  redChi2Ped = new TH1F("redChi2Ped", "Reduced #chi^{2} pedestals fit;#chi^{2} / NDF;Entries", 200, 0, 20);
  redChi2Noise = new TH1F("redChi2Noise", "Reduced #chi^{2} noise fit;#chi^{2} / NDF;Entries", 200, 0, 20);

  commModeGr = new TGraph();
  commModeGr->SetName("commModeGr");
  commModeGr->SetTitle("Common mode vs. event");
}

PedRun::~PedRun()
{
}

void PedRun::doSpecificStuff() // specific version of the function from parent class
{
  for(int i = 0; i < nChannels; ++i) // fill pedestal histos
    pedHist[i]->Fill(adcPH[i]);

  return;
}

double PedRun::CommonModeCalculation(double* phChannels)
{
  double sum = 0;
  for(unsigned int i = 0; i < goodChannels.size(); ++i)
    sum += phChannels[goodChannels[i]];

  return sum / goodChannels.size();
}

void PedRun::computeNoise()
{
  UInt_t PH[nChannels];
  double pedSubPH[nChannels];
  Float_t commMode;
  long int nEntries;

  rawEvtTree->SetBranchStatus("*", 0); // deactivate all branches
  rawEvtTree->SetBranchStatus("adcPH", 1); // activate this branch
  rawEvtTree->SetBranchAddress("adcPH", PH);

 TBranch* commBr = rawEvtTree->Branch("commMode", &commMode, "commMode/F"); // new branch

  nEntries = rawEvtTree->GetEntries();

  for(long int iEvt = 0; iEvt < nEntries; ++iEvt)
    {
      rawEvtTree->GetEntry(iEvt);

      for(int iCh = 0; iCh < nChannels; ++iCh) // pedestal subtraction
	pedSubPH[iCh] = PH[iCh] - pedestals[iCh];

      commMode = CommonModeCalculation(pedSubPH);
      commModeGr->SetPoint(iEvt, iEvt, commMode);

      commBr->Fill(); // fill just this branch

      for(unsigned int iHist = 0; iHist < noiseHist.size(); ++iHist)
	noiseHist[iHist]->Fill(pedSubPH[goodChannels[iHist]] - commMode);

    }

  rawEvtTree->SetBranchStatus("*", 1); // activate all branches

  TF1* gaFit;
  TCanvas* servCan = new TCanvas();

  for(unsigned int iHist = 0; iHist < noiseHist.size(); ++iHist)
    {
      gaFit = new TF1("gaFit", "gaus");
      noiseHist[iHist]->Fit(gaFit, "Q");

      NoiseGraph->SetPoint(iHist, goodChannels[iHist], gaFit->GetParameter(2));
      NoiseGraph->SetPointError(iHist, 0, gaFit->GetParError(2));

      noise[goodChannels[iHist]] = gaFit->GetParameter(2);

      redChi2Noise->Fill(gaFit->GetChisquare() / gaFit->GetNDF());

      delete gaFit;
    }

  delete servCan;

  return;
}

void PedRun::fitPedestals()
{
  TF1* gaFit;
  TCanvas* servCan = new TCanvas();


  for(int i = 0; i < nChannels; ++i)
    {
      gaFit = new TF1("gaFit", "gaus");
      pedHist[i]->Fit(gaFit, "Q");

      PedGraph->SetPoint(i, i, gaFit->GetParameter(1));
      PedGraph->SetPointError(i, 0, gaFit->GetParError(1));

      RawNoiseGraph->SetPoint(i, i, gaFit->GetParameter(2));
      RawNoiseGraph->SetPointError(i, 0, gaFit->GetParError(2));

      redChi2Ped->Fill(gaFit->GetChisquare() / gaFit->GetNDF());

      pedestals[i] = gaFit->GetParameter(1);

      delete gaFit;
    }

  delete servCan;

  return;
}

void PedRun::writePedList()
{
  char fileName[300];
  sprintf(fileName, "%s/PedNoise%06d.list", outFilePath.c_str(), runNumber);
  std::ofstream pedList;
  pedList.open(fileName);

  for(int i = 0; i < nChannels; ++i)
    pedList << i << '\t' << pedestals[i] << '\t' << noise[i] << '\n';

  pedList.close();

  return;
}

void PedRun::writeHistos()
{
  pedDir->cd();
  for(int i = 0; i < nChannels; ++i)
    pedHist[i]->Write();

  noiseDir->cd();
  for(unsigned int i = 0; i < noiseHist.size(); ++i)
    noiseHist[i]->Write();

  outFile->cd();

  TCanvas* servCan = new TCanvas();
  servCan->cd();

  PedGraph->Draw("AP");
  PedGraph->GetXaxis()->SetTitle("Channel");
  PedGraph->GetXaxis()->SetRangeUser(-0.5, 255.5);
  PedGraph->GetYaxis()->SetTitle("Pedestal [ADC]");
  PedGraph->GetYaxis()->SetRangeUser(0, 1024);
  PedGraph->Write();

  RawNoiseGraph->Draw("AP");
  RawNoiseGraph->GetXaxis()->SetTitle("Channel");
  RawNoiseGraph->GetXaxis()->SetRangeUser(-0.5, 255.5);
  RawNoiseGraph->GetYaxis()->SetTitle("Noise [ADC]");
  RawNoiseGraph->GetYaxis()->SetRangeUser(0, 50);
  RawNoiseGraph->Write();

  NoiseGraph->Draw("AP");
  NoiseGraph->GetXaxis()->SetTitle("Channel");
  NoiseGraph->GetXaxis()->SetRangeUser(-0.5, 255.5);
  NoiseGraph->GetYaxis()->SetTitle("Noise [ADC]");
  NoiseGraph->GetYaxis()->SetRangeUser(0, 20);
  NoiseGraph->Write();

  commModeGr->Draw("AP");
  commModeGr->GetXaxis()->SetTitle("Event");
  commModeGr->GetYaxis()->SetTitle("Common mode [ADC]");
  commModeGr->Write();

  delete servCan;

  redChi2Ped->Write();
  redChi2Noise->Write();

  return;
}

void PedRun::analyseRunHeader()
{
  int posPipe = runHeader.find('|');
  int posSemiCol = runHeader.find(';');
  posPipe += 1;

  expectedEvts = atoi(runHeader.substr(posPipe, posSemiCol - posPipe).c_str());

  std::istringstream headStr;
  headStr.str(runHeader);

  std::string trash;
  int transBlock;
  headStr >> trash >> transBlock;

  std::cout << "Expected " << expectedEvts << " events, transferred in blocks of " << transBlock << std::endl;

  return;
}

void PedRun::AnalyseData()
{
  fitPedestals();
  computeNoise();
  writeHistos();
  writePedList();

  return;
}
