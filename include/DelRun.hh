#ifndef DELRUN_HH
#define DELRUN_HH

#include "constants.h"
#include "BinaryData.hh"

#include "TH2F.h"
#include "TProfile.h"

class ConfigFileReader;

class DelRun : public BinaryData
{
  TDirectory* histoDir[2]; // [2] cause of the different polarities
  TDirectory* profileDir[2];

  TH2F* delHistos[nChannels][2]; // [2] cause of the different polarities
  TProfile* delProfiles[nChannels][2];

  TH2F* delHistosGoodCh[2]; // sum of histograms of the good channels
  TProfile* delProfileGoodCh[2];

  int startDel; // start value of the delay
  int endDel; // end value of the delay
  int stepSize; // step size from start to end
  int nSamples; // measurements for each step for each polarity

  int iSample; // counter to associate the measured charge to the right delay

  void ReadPedFile(const char* pedFile);
  Float_t pedestals[nChannels];
  Float_t noise[nChannels];

  void doSpecificStuff();
  void analyseRunHeader();

  void createHistos();
  void writeHistos();
  void writeProfiles();

public:
  DelRun(const char* binFile, ConfigFileReader* Conf);
  ~DelRun();

  void AnalyseData();

};

#endif // #ifndef DELRUN_HH

