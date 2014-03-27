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
  cookedEvtTree->Branch("commMode", commMode, "commMode[2]/F");
  cookedEvtTree->Branch("clustVec", "vector<cluster>", &clustVecPtr);
  cookedEvtTree->Branch("signal", signal, TString::Format("signal[%i]/F", nChannels));

  // ==================== plots ===============================
  chInCommMode = new TH1I("chInCommMode", "Number of channels used in common mode calculation after cuts;Number of channels;Events", 256, -0.5, 255.5);

  commVsEvtOffset = new TGraph();
  commVsEvtOffset->SetName("commVsEvtOffset");
  commVsEvtOffset->SetTitle("Common mode offset vs event number");

  commVsEvtSlope = new TGraph();
  commVsEvtSlope->SetName("commVsEvtSlope");
  commVsEvtSlope->SetTitle("Common mode slope vs event number");

  clusterSize = new TH1I("clusterSize", "Cluster size;Number of channels", 256, -0.5, 255.5);
  nClustEvt = new TH1I("nClustEvt", "Number of clusters per event;Number of clusters", 101, -0.5, 100.5);

  signalTime = new TH2D("signalTime", "Signal vs time;Time [ns];Signal [ADC]", 60, 0, 120, 1024, -511.5, 511.5);
  clusterTime = new TH2D("clusterTime", "Cluster charge vs time;Time [ns];Cluster charge [ADC]", 60, 0, 120, 1024, -511.5, 511.5);
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
  // common mode = a + b * chNum
  double S = 0;
  double Sx = 0;
  double Sy = 0;
  double Sxx = 0;
  double Sxy = 0;
  double Delta = 0;
  double a = 0;
  double b = 0;
  // double sig2a = 0; // sigma squared of the variables
  // double sig2b = 0;
  // double covab = 0; // covariance

  double sigma = 0; // variance centered on the fitted line
  double sumDevSq = 0; // sum to calculate the variance

  std::vector<int> chInUse = goodChannels;
  std::vector<int> chPrev; // channels used in the previous step
  int chNum = 0;

  for(int iIter = 0; iIter < nCMiter; ++iIter)
    {
      // put variables to 0
      S = 0;
      Sx = 0;
      Sy = 0;
      Sxx = 0;
      Sxy = 0;

      // all the weights for the linear regression assumed to be 1
      S = chInUse.size();

      for(unsigned int iCh = 0; iCh < chInUse.size(); ++iCh) // sums for the cm calculation
	{
	  Sx += chInUse[iCh];
	  Sy += phChannels[chInUse[iCh]];
	  Sxx += chInUse[iCh] * chInUse[iCh];
	  Sxy += chInUse[iCh] * phChannels[chInUse[iCh]];
	}

      Delta = S * Sxx - Sx * Sx;
      a = (Sxx * Sy - Sx * Sxy) / Delta; // a
      b = (S * Sxy - Sx * Sy) / Delta; // b
      // sig2a = Sxx / Delta;
      // sig2b = S / Delta;
      // covab = -Sx / Delta;

      // std::cout << a << "  " << b << "  " << sig2a << "  " << sig2b << "  " << covab << std::endl;

      for(unsigned int iCh = 0; iCh < chInUse.size(); ++iCh) // sum for variance, calculated around the fitted line
	sumDevSq += pow(phChannels[chInUse[iCh]] - a - b * chInUse[iCh], 2);

      sigma = sqrt(sumDevSq / (chInUse.size() - 1));
 
      if(iIter < nCMiter - 1) // last iteration does not need this
	{
	  chPrev = chInUse;
	  chInUse.clear();
	  
	  for(unsigned int iCh = 0; iCh < chPrev.size(); ++iCh) // select channels
	    {
	      chNum = chPrev[iCh];
	      // if the distance between the ph and the estimated cm is more than sigCut times the cm uncertainty the channel is excluded
	      // if(fabs(phChannels[chNum] - a - b * chNum) 
	      // 	 < sigCut * sqrt(sig2a + sig2b * chNum * chNum + 2 * chNum * covab)) chInUse.push_back(chNum);

	      if(fabs(phChannels[chNum] - a - b * chNum) 
		 < sigCut * sigma) chInUse.push_back(chNum);
	    }
	}
    }

  commMode[0] = a;
  commMode[1] = b;

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

  int evtNum = commVsEvtOffset->GetN();
  commVsEvtOffset->SetPoint(evtNum, evtNum, commMode[0]);
  commVsEvtSlope->SetPoint(evtNum, evtNum, commMode[1]);

  for(unsigned int iCh = 0; iCh < goodChannels.size(); ++iCh) // common mode subtraction
    signal[goodChannels[iCh]] = pedSubPH[goodChannels[iCh]] - commMode[0] - commMode[1] * goodChannels[iCh];

  FindClusters(signal);

  cookedEvtTree->Fill();

  return;
}

void DataRun::AnalyseData()
{
  std::vector<cluster>* cluVecPtr = NULL;
  Float_t time;
  Float_t sig[nChannels] = {0};

  cookedEvtTree->SetBranchAddress("clustVec", &cluVecPtr);
  cookedEvtTree->SetBranchAddress("signal", sig);
  rawEvtTree->SetBranchAddress("time", &time);

  cookedEvtTree->SetBranchStatus("*", 0); // deactivate all the branches
  cookedEvtTree->SetBranchStatus("clustVec*", 1); // activate clustVec and all the members of the struct, the sintax with the * is fundamental
  cookedEvtTree->SetBranchStatus("signal", 1);

  rawEvtTree->SetBranchStatus("*", 0);
  rawEvtTree->SetBranchStatus("time", 1);

  long int nEntries = cookedEvtTree->GetEntries();

  int nClust;
  cluster clu;

  for(long int i = 0; i < nEntries; ++i)
    {
      rawEvtTree->GetEntry(i);
      cookedEvtTree->GetEntry(i);

      for(unsigned int iCh = 0; iCh < goodChannels.size(); ++iCh) signalTime->Fill(time, sig[goodChannels.at(iCh)]);

      nClust = cluVecPtr->size();
      nClustEvt->Fill(nClust);

      if(nClust != 0)
      	for(int iCl = 0; iCl < nClust; iCl++)
      	  {
      	    clu = cluVecPtr->at(iCl);
	    clusterTime->Fill(time, clu.adcTot);
      	  }
    }

  signalTimeProfile = signalTime->ProfileX("signalTimeProfile");
  signalTimeProfile->SetTitle("Time profile of the signal;Time [ns];Signal [ADC]");

  clusterTimeProfile = clusterTime->ProfileX("clusterTimeProfile");
  clusterTimeProfile->SetTitle("Time profile of the cluster charge;Time [ns];Cluster charge [ADC]");

  outFile->cd();
  nClustEvt->Write();
  signalTime->Write();
  signalTimeProfile->Write();
  clusterTime->Write();
  clusterTimeProfile->Write();

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

  commVsEvtOffset->Draw("AP");
  commVsEvtOffset->GetXaxis()->SetTitle("Event number");
  commVsEvtOffset->GetYaxis()->SetTitle("Offset[ADC]");
  commVsEvtOffset->Write();

  commVsEvtSlope->Draw("AP");
  commVsEvtSlope->GetXaxis()->SetTitle("Event number");
  commVsEvtSlope->GetYaxis()->SetTitle("Slope[ADC / Ch. number]");
  commVsEvtSlope->Write();

  delete serv;

  clusterSize->Write();

  return;
}

void DataRun::AddStrip(cluster* clu, Float_t* phArray, int stripNum)
{
  clu->adcTot += phArray[stripNum]; // the total charge is the sum with sign
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
  bool usedStrip[nChannels] = {0}; // true if a strip has been used, to prevent same strip in multiple clusters

  for(unsigned int iCh = 0; iCh < goodChannels.size(); ++iCh) // find seeds
    {
      chNum = goodChannels[iCh]; // take a good channel

      if(phChannels[chNum] / fabs(phChannels[chNum]) == polarity / fabs(polarity) &&
	 fabs(phChannels[chNum] / noise[chNum]) > snrSeed) // seeds must have the right polarity and pass the seed cut
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
      if(usedStrip[seeds[iSd]] == true) continue; // if the seed has alredy been used

      clus = new cluster();
      usedStrip[seeds[iSd]] = true; // mark the strip as used
      AddStrip(clus, phChannels, seeds[iSd]); // add the seed to the cluster

      growLeft = true;
      growRight = true;

      iNgh = 1;

      do // while(growLeft || growRight)
	{
	  if(growLeft)
	    {
	      chNum = seeds[iSd] - iNgh; // left direction
	      if(chNum < 0 || noise[chNum] == -1) // channel out of margin or it is a bad channel
		growLeft = false;
	      else
		if(fabs(phChannels[chNum] / noise[chNum]) > snrNeigh && usedStrip[chNum] == false) // check for snr and that the strip has not already been used
		  {
		    if(iNgh == 1 || isSeed[chNum] == false) // if the first neighbour of the seed is another seed add it, otherwise don't
		      {
			usedStrip[chNum] = true;
			AddStrip(clus, phChannels, chNum); // add strip to the cluster
		      }
		  }
		else
		  growLeft = false;
	    }

	  if(growRight)
	    {
	      chNum = seeds[iSd] + iNgh; // right direction
	      if(chNum >= nChannels || noise[chNum] == -1) // channel out of margin or it is a bad channel
		growRight = false;
	      else
		if(fabs(phChannels[chNum] / noise[chNum]) > snrNeigh && usedStrip[chNum] == false) // check for snr and that the strip has not already been used
		  {
		    if(iNgh == 1 || isSeed[chNum] == false) // if the first neighbour of the seed is another seed add it, otherwise don't
		      {
			usedStrip[chNum] = true;
			AddStrip(clus, phChannels, chNum); // add strip to the cluster
		      }
		  }
		else
		  growRight = false;
	    }

	  iNgh++;
	} while(growLeft || growRight);

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
  double sumADC = 0;

  for(unsigned int iStr = 0; iStr < clu->strips.size(); ++iStr)
    {
      sumPos += clu->strips.at(iStr) * fabs(clu->adcStrips.at(iStr));
      sumADC += fabs(clu->adcStrips.at(iStr));
    }

  clu->posStrAdc = sumPos / sumADC;
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
