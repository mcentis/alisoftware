#ifndef PEDRUN_HH
#define PEDRUN_HH

#include "constants.h"
#include "BinaryData.hh"

#include "TH1F.h"
#include "TH1I.h"
#include "TGraphErrors.h"

class ConfigFileReader;

class PedRun : public BinaryData
{
  TDirectory* pedDir;
  TDirectory* noiseDir;
  double pedestals[nChannels];
  double noise[nChannels];
  TH1I* pedHist[nChannels];
  std::vector<TH1F*> noiseHist;
  TGraphErrors* PedGraph;
  TGraphErrors* RawNoiseGraph;
  TGraphErrors* NoiseGraph;
  TH1F* redChi2Ped;
  TH1F* redChi2Noise;
  void fitPedestals();
  void writeHistos();
  void computeNoise();
  double CommonModeCalculation(double* phChannels); // this takes the pedestal subtracted ph, it is assumed that no signal is present
  TGraph* commModeGr;
  void writePedList();

  void doSpecificStuff();
  void analyseRunHeader();

public:
  PedRun(const char* binFile, ConfigFileReader* Conf);
  ~PedRun();

  void AnalyseData();
};

#endif // #ifndef PEDRUN_HH