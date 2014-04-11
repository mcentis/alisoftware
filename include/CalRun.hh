#ifndef CALRUN_HH
#define CALRUN_HH

#include "constants.h"
#include "BinaryData.hh"

#include "TH2F.h"
#include "TProfile.h"

class ConfigFileReader;

class CalRun : public BinaryData
{
  TDirectory* histoDir;
  TDirectory* profileDir;
  double parameters[nChannels][nParameters];
  TH2F* calHistos[nChannels];
  TProfile* calProfiles[nChannels];

  int startCharge; // min abs val of injected charge
  int endCharge; // max abs val of injected charge
  int stepSize; // steps size from min to max
  int nSamples; // measurements for each step for each polarity

  // counters to associate the measured charge to the right injection
  int iSample;
  int iStep;

  int injCharge; // value of charge injected in one test

  void doSpecificStuff();
  void analyseRunHeader();

  void createHistos();
  void writeHistos();
  void writeProfiles();

public:
  CalRun(const char* binFile, ConfigFileReader* Conf);
  ~CalRun();

  void AnalyseData();
};

#endif // #ifndef CALRUN_HH
