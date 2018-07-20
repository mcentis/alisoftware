#ifndef CALRUN_HH
#define CALRUN_HH

#include "constants.h"
#include "BinaryData.hh"

#include "TGraph.h"
#include "TH2F.h"
#include "TProfile.h"

class ConfigFileReader;

class CalRun : public BinaryData
{
  TDirectory* histoDir;
  TDirectory* profileDir;
  Double_t parameters[nChannels][nParameters]; // one fit for both positive and negative signals
  TH2F* calHistos[nChannels];
  TProfile* calProfiles[nChannels];

  bool isGoodCh[nChannels]; // true if the channel is good

  int startCharge; // min abs val of injected charge
  int endCharge; // max abs val of injected charge
  int stepSize; // steps size from min to max
  int nSamples; // measurements for each step for each polarity

  // counters to associate the measured charge to the right injection
  int iSample;
  // int iStep;

  // int charge; // value of charge injected in one test

  void ReadPedFile(const char* pedFile);
  Float_t pedestals[nChannels];
  Float_t noise[nChannels];

  void fitCalibrations();
  float startFit;
  float endFit;

  TH1F* redChi2Cal;

  TDirectory* parCal;

  TGraph* parVsCh[nParameters];

  TH1F* parDistr[nParameters];

  TH1F* parDistr_goodCh[nParameters]; // same as the ones before, just for good channels

  TGraph* redChi2vsCh;

  void doSpecificStuff();
  void analyseRunHeader();

  void createHistos();
  void writeHistos();
  void drawParGraphs();
  void writeParGraphs();
  void writeProfiles();
  void writeParList();

public:
  CalRun(const char* binFile, ConfigFileReader* Conf);
  ~CalRun();

  void AnalyseData();
};

#endif // #ifndef CALRUN_HH
