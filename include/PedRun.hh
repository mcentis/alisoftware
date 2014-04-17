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
  void CommonModeCalculation(double* phChannels, Float_t* res); // this takes the pedestal subtracted ph, it is assumed that no signal is present, cm with slope!!
  TGraph* commModeGrSlope;
  TGraph* commModeGrOffset;
  TH1F* commModeSlopeDistr;
  TH1F* commModeOffsetDistr;
  void writePedList();

  void doSpecificStuff();
  void analyseRunHeader();

public:
  PedRun(const char* binFile, ConfigFileReader* Conf);
  ~PedRun();

  void AnalyseData();
};

#endif // #ifndef PEDRUN_HH
