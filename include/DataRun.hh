#ifndef DATARUN_HH
#define DATARUN_HH

#include "constants.h"
#include "clusterDef.h"
#include "BinaryData.hh"

class TH1I;
class TProfile;
class TH2D;
class TGraph;
class TF1;

class DataRun : public BinaryData
{
  TTree* chPropTree; // properties of the channels: pedestal, noise, calibration. it has just one event
  TTree* cookedEvtTree; // elaborated events

  void ReadPedFile(const char* pedFile);
  Float_t pedestals[nChannels];
  Float_t noise[nChannels];

  void ReadCalFile(const char* calFile);
  Double_t calibrations[nChannels][2][nParameters]; // 2 polarities
  TF1* toAliE; // function to convert the adc info to alibava electrons

  void CommonModeCalculation(double* phChannels); // this takes the pedestal subtracted ph, common mode as line with slope, cuts for signal exclusion
  void FindClusters(Float_t* phChannels); // this takes pedestal and common mode subtracted ph, the non used channels ph set to 0

  void FindClusterPos(cluster* clu); // find the position of the cluster using the center of gravity algorithm

  void AddStrip(cluster* clu, Float_t* phArray, int stripNum); // add strip to cluster, phArray is the array with the signal of the event

  double sigCut; // cut on sigma for common mode calculation
  int nCMiter; // number of iterations in the common mode algorithm

  double snrSeed; // snr seed and neighbour strips for clustering
  double snrNeigh;

  int polarity; // polarity of the expected signal, used only in the seed finding

  double pitch; // pitch of the sensor, in mm

  // branches of the elaborated events tree
  Float_t commMode[2];
  std::vector<cluster> clustVec;
  std::vector<cluster>* clustVecPtr;
  Float_t signal[nChannels];
  Float_t caliSignal[nChannels];

  TH1I* chInCommMode; // number of channels in the common mode calculation
  TGraph* commVsEvtOffset; // common mode versus event number
  TGraph* commVsEvtSlope; // common mode versus event number
  TH1F* commModeSlopeDistr;
  TH1F* commModeOffsetDistr;

  TH1I* clusterSize; // number of channels used in each cluster
  TH1I* nClustEvt; // number of clusters per event

  TH2D* signalTime;
  TProfile* signalTimeProfile;

  TH2D* clusterTime;
  TProfile* clusterTimeProfile;

  void doSpecificStuff();
  void analyseRunHeader();

public:
  DataRun(const char* binFile, ConfigFileReader* Conf);
  ~DataRun();

  void AnalyseData();
  void WriteCookedTree();
};

#endif // #ifndef DATARUN_HH
