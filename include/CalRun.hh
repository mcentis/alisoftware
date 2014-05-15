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
  Double_t parameters[nChannels][2][nParameters]; // the [2] dimension is there since there is a calibration for positive (1) and negative (0) values
  TH2F* calHistos[nChannels];
  TProfile* calProfiles[nChannels];

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

  TDirectory* parPosCal;
  TDirectory* parNegCal;

  TGraph* parVsCh_posCal[nParameters];
  TGraph* parVsCh_negCal[nParameters];

  TH1F* parDistr_posCal[nParameters];
  TH1F* parDistr_negCal[nParameters];

  TGraph* redChi2vsCh_posCal;
  TGraph* redChi2vsCh_negCal;

  TGraph* meanSignalEvt[nChips];

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
