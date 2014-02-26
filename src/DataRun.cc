#include "DataRun.hh"

#include "iostream"
#include "string"
#include "fstream"
#include "sstream"
#include "stdlib.h"
#include "math.h"

#include "TString.h"
#include "TH1I.h"
#include "TProfile.h"
#include "TH2D.h"
#include "TGraph.h"
#include "TCanvas.h"

DataRun::DataRun(const char* binFile, ConfigFileReader* Conf):
  BinaryData(binFile, Conf)
{
  for(int i = 0; i < nChannels; ++i)
    {
      pedestals[i] = -1;
      noise[i] = -1;
    }

  for(int iCh = 0; iCh < nChannels; ++iCh)
    for(int iPar = 0; iPar < nParameters; ++iPar)
      calibrations[iCh][iPar] = 0;

  sigCut = atof(conf->GetValue("sigCut").c_str());
  nCMiter = atoi(conf->GetValue("nCMiter").c_str());

  snrSeed = atof(conf->GetValue("snrSeed").c_str());
  snrNeigh = atof(conf->GetValue("snrNeigh").c_str());

  polarity = atoi(conf->GetValue("polarity").c_str());

  pitch = atof(conf->GetValue("pitch").c_str());

  readGoodChFile(conf->GetValue("goodChFile").c_str());

  // ========================= channel properties ==================
  chPropTree = new TTree("chPropTree", "Channel properties");
  chPropTree->Branch("pedestals", pedestals, TString::Format("pedestals[%i]/F", nChannels)); // trick with the tstring to adjust the lenght of the array
  chPropTree->Branch("noise", noise, TString::Format("noise[%i]/F", nChannels));
  chPropTree->Branch("calibrations", calibrations, TString::Format("calibrations[%i][%i]/F", nChannels, nParameters));

  ReadPedFile(conf->GetValue("pedNoiseFile").c_str()); // read pedestal file
  ReadCalFile(conf->GetValue("calFile").c_str()); // read calibration file

  chPropTree->Fill();
  outFile->cd();
  chPropTree->Write();

  // ====================== elaborated events =================
  clustVecPtr = &clustVec;
  cookedEvtTree = new TTree("cookedEvtTree", "Elaborated data");
  cookedEvtTree->Branch("commMode", &commMode, "commMode/F");
  cookedEvtTree->Branch("clustVec", "vector<cluster>", &clustVecPtr);
  cookedEvtTree->Branch("signal", signal, TString::Format("signal[%i]/F", nChannels));

  // ==================== plots ===============================
  chInCommMode = new TH1I("chInCommMode", "Number of channels used in common mode calculation after cuts;Number of channels;Events", 256, -0.5, 255.5);

  commVsEvt = new TGraph();
  commVsEvt->SetName("commVsEvt");
  commVsEvt->SetTitle("Common mode vs event number");

  clusterSize = new TH1I("clusterSize", "Cluster size;Number of channels", 256, -0.5, 255.5);
  nClustEvt = new TH1I("nClustEvt", "Number of clusters per event;Number of clusters", 101, -0.5, 100.5);

  signalTime = new TH2D("signalTime", "Cluster charge vs time;Time [ns];Cluster charge [ADC]", 60, 0, 120, 1024, -0.5, 1023.5);
}

DataRun::~DataRun()
{
}

void DataRun::ReadPedFile(const char* pedFile)
{
  ifstream pedStr;
  pedStr.open(pedFile, std::ifstream::in);

  if(pedStr.is_open() == false)
    {
      std::cout << "[DataRun::ReadPedFile] Impossile to open " << pedFile << " all pedestals and noises set to -1" << std::endl;
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

void DataRun::ReadCalFile(const char* calFile)
{
  ifstream calStr;
  calStr.open(calFile, std::ifstream::in);

  if(calStr.is_open() == false)
    {
      std::cout << "[DataRun::ReadcalFile] Impossile to open " << calFile << " all parameters set to 0" << std::endl;
      for(int iCh = 0; iCh < nChannels; ++iCh)
	for(int iPar = 0; iPar < nParameters; ++iPar)
	  calibrations[iCh][iPar] = 0;

      return;
    }

  std::string line;
  std::istringstream lineStr;

  int chNum;

  while(calStr.good())
    {
      std::getline(calStr, line);

      lineStr.clear();
      lineStr.str(line);

      lineStr >> chNum;

	for(int iPar = 0; iPar < nParameters; ++iPar)
	  lineStr >> calibrations[chNum][iPar];
    }

  calStr.close();

  return;
}

void DataRun::CommonModeCalculation(double* phChannels)
{
  double sum;
  double sumDevSq;
  double mean = 0; // put here, if there is no calculation the commMode stays at 0
  double sigma;

  std::vector<int> chInUse = goodChannels;
  std::vector<int> chPrev; // channels used in the previous step

  for(int iIter = 0; iIter < nCMiter; ++iIter)
    {
      sum = 0;
      sumDevSq = 0;

      for(unsigned int iCh = 0; iCh < chInUse.size(); ++iCh) // sum for mean
	  sum += phChannels[chInUse[iCh]];

      mean = sum / chInUse.size();

      for(unsigned int iCh = 0; iCh < chInUse.size(); ++iCh) // sum for rms
	sumDevSq += pow(phChannels[chInUse[iCh]] - mean, 2);

      sigma = sqrt(sumDevSq / (chInUse.size() - 1));
      
      if(iIter < nCMiter - 1) // last iteration does not need this
	{
	  chPrev = chInUse;
	  chInUse.clear();
	  
	  for(unsigned int iCh = 0; iCh < chPrev.size(); ++iCh) // select channels
	    if(fabs(phChannels[chPrev[iCh]] - mean) < sigma * sigCut) chInUse.push_back(chPrev[iCh]);
	}
    }

  commMode = mean;

  chInCommMode->Fill(chInUse.size());

  return;
}

void DataRun::doSpecificStuff()
{
  double pedSubPH[nChannels] = {0};
  for(int i = 0; i < nChannels; ++i) signal[i] = 0; // reset to 0 the signal

  for(unsigned int iCh = 0; iCh < goodChannels.size(); ++iCh) // pedestal subtraction
    pedSubPH[goodChannels[iCh]] = adcPH[goodChannels[iCh]] - pedestals[goodChannels[iCh]];

  CommonModeCalculation(pedSubPH);

  commVsEvt->SetPoint(commVsEvt->GetN(), commVsEvt->GetN(), commMode);

  for(unsigned int iCh = 0; iCh < goodChannels.size(); ++iCh) // common mode subtraction
    signal[goodChannels[iCh]] = pedSubPH[goodChannels[iCh]] - commMode;

  FindClusters(signal);

  cookedEvtTree->Fill();

  return;
}

void DataRun::AnalyseData()
{
  std::vector<cluster>* cluVecPtr = NULL;
  Float_t time;

  cookedEvtTree->SetBranchAddress("clustVec", &cluVecPtr);
  rawEvtTree->SetBranchAddress("time", &time);

  cookedEvtTree->SetBranchStatus("*", 0); // deactivate all the branches
  cookedEvtTree->SetBranchStatus("clustVec*", 1); // activate clustVec and all the members of the struct, the sintax with the * is fundamental

  rawEvtTree->SetBranchStatus("*", 0);
  rawEvtTree->SetBranchStatus("time", 1);

  long int nEntries = cookedEvtTree->GetEntries();

  int nClust;
  cluster clu;

  for(long int i = 0; i < nEntries; ++i)
    {
      rawEvtTree->GetEntry(i);
      cookedEvtTree->GetEntry(i);

      nClust = cluVecPtr->size();
      nClustEvt->Fill(nClust);

      if(nClust != 0)
      	for(int iCl = 0; iCl < nClust; iCl++)
      	  {
      	    clu = cluVecPtr->at(iCl);
	    signalTime->Fill(time, clu.adcTot);
      	  }
    }

  timeProfile = signalTime->ProfileX("timeProfile");
  timeProfile->SetTitle("Time profile of the signal;Time [ns];Signal [ADC]");

  outFile->cd();
  nClustEvt->Write();
  signalTime->Write();
  timeProfile->Write();

  rawEvtTree->SetBranchStatus("*", 1); // reactivate all the branches
  cookedEvtTree->SetBranchStatus("*", 1);

  return;
}

void DataRun::WriteCookedTree()
{
  outFile->cd();
  cookedEvtTree->Write();

  chInCommMode->Write();

  TCanvas* serv = new TCanvas();

  commVsEvt->Draw("AP");
  commVsEvt->GetXaxis()->SetTitle("Event number");
  commVsEvt->GetYaxis()->SetTitle("Common mode [ADC]");
  commVsEvt->Write();

  delete serv;

  clusterSize->Write();

  return;
}

void DataRun::AddStrip(cluster* clu, Float_t* phArray, int stripNum)
{
  clu->adcTot += fabs(phArray[stripNum]); // the total charge is defined as positive
  clu->strips.push_back(stripNum);
  clu->adcStrips.push_back(phArray[stripNum]);

  return;
}

void DataRun::FindClusters(Float_t* phChannels)
{
  clustVec.clear();

  static long evtNum = -1;
  evtNum++;

  std::vector<int> seeds;
  cluster* clus;

  int chNum; // channel number used
  bool isSeed[nChannels] = {0}; // true if the channel is a seed

  for(unsigned int iCh = 0; iCh < goodChannels.size(); ++iCh) // find seeds
    {
      chNum = goodChannels[iCh]; // take a good channel

      if(phChannels[chNum] / noise[chNum] * polarity > snrSeed) // only seeds of the right polarity are accepted
	{
	  seeds.push_back(chNum);
	  isSeed[chNum] = true;
	}
    }

  bool growLeft; // bools to control the growth of the cluster
  bool growRight;

  int iNgh; // distance from the seed

  for(unsigned int iSd = 0; iSd < seeds.size(); ++iSd) // create and grow the cluster
    {
      clus = new cluster();
      AddStrip(clus, phChannels, seeds[iSd]); // add the seed to the cluster

      growLeft = true;
      growRight = true;

      iNgh = 1;

      while(growLeft || growRight)
	{
	  if(growLeft)
	    {
	      chNum = seeds[iSd] - iNgh; // left direction
	      if(chNum < 0)
		growLeft = false;
	      else
		if(phChannels[chNum] / noise[chNum] > snrNeigh && isSeed[chNum] == false) // check for snr and that the strip is not a seed, bad channels excluded because noise = -1 for them
		  AddStrip(clus, phChannels, chNum); // add strip to the cluster
		else
		  growLeft = false;
	    }

	  if(growRight)
	    {
	      chNum = seeds[iSd] + iNgh; // right direction
	      if(chNum >= nChannels)
		growRight = false;
	      else
		if(phChannels[chNum] / noise[chNum] > snrNeigh && isSeed[chNum] == false) // check for snr and that the strip is not a seed, bad channels excluded because noise = -1 for them
		  AddStrip(clus, phChannels, chNum); // add strip to the cluster
		else
		  growRight = false;
	    }

	  iNgh++;
	}

      FindClusterPos(clus); // calculate the position

      clusterSize->Fill(clus->strips.size());

      clustVec.push_back(*clus);
      delete clus;
    }

  return;
}

void DataRun::FindClusterPos(cluster* clu)
{
  double sumPos = 0;

  for(unsigned int iStr = 0; iStr < clu->strips.size(); ++iStr)
    sumPos += clu->strips.at(iStr) * fabs(clu->adcStrips.at(iStr));

  clu->posStrAdc = sumPos / clu->adcTot;
  clu->posmmAdc = clu->posStrAdc * pitch;

  return;
}

void DataRun::analyseRunHeader()
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