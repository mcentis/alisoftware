#include "DataRun.hh"

#include "iostream"
#include "string"
#include "fstream"
#include "sstream"
#include "stdlib.h"
#include "math.h"

#include "TString.h"
#include "TH1I.h"
#include "TF1.h"
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

  toAliE = new TF1("toAliE", convertToe);

  sigCut = atof(conf->GetValue("sigCut").c_str());
  nCMiter = atoi(conf->GetValue("nCMiter").c_str());

  snrSeed = atof(conf->GetValue("snrSeed").c_str());
  snrNeigh = atof(conf->GetValue("snrNeigh").c_str());

  polarity = atoi(conf->GetValue("polarity").c_str());
  polarity /= abs(polarity); // make the polarity be a unit with sign

  pitch = atof(conf->GetValue("pitch").c_str());

  if(atof(conf->GetValue("scaleFactor").c_str()) != 0)
    {
      scaleFactor = atof(conf->GetValue("scaleFactor").c_str());
      std::cout << "================================================================>>> WARNING You are going to apply a scaling factor to the signal after the clustering (in the AddStrip function)" <<std::endl;
    }
  else
    scaleFactor = 1;

  readGoodChFile(conf->GetValue("goodChFile").c_str());

  // ========================= channel properties ==================
  chPropTree = new TTree("chPropTree", "Channel properties");
  chPropTree->Branch("pedestals", pedestals, TString::Format("pedestals[%i]/F", nChannels)); // trick with the tstring to adjust the lenght of the array
  chPropTree->Branch("noise", noise, TString::Format("noise[%i]/F", nChannels));
  chPropTree->Branch("calibrations", calibrations, TString::Format("calibrations[%i][%i]/D", nChannels, nParameters));

  ReadPedFile(conf->GetValue("pedNoiseFile").c_str()); // read pedestal file
  ReadCalFile(conf->GetValue("calFile").c_str()); // read calibration file

  chPropTree->Fill();
  outFile->cd();
  chPropTree->Write();

  // ====================== elaborated events =================
  clustVecPtr = &clustVec;
  cookedEvtTree = new TTree("cookedEvtTree", "Elaborated data");
  cookedEvtTree->Branch("time", &time, "time/F");
  cookedEvtTree->Branch("temp", &temp, "temp/F");
  cookedEvtTree->Branch("commMode", commMode, TString::Format("commMode[%i][2]/F", nChips));
  cookedEvtTree->Branch("clustVec", "vector<cluster>", &clustVecPtr);
  cookedEvtTree->Branch("signal", signal, TString::Format("signal[%i]/F", nChannels));
  cookedEvtTree->Branch("caliSignal", caliSignal, TString::Format("caliSignal[%i]/F", nChannels));

  // ==================== plots ===============================
  char name[50];
  char title[200];

  chInCommMode = new TH1I("chInCommMode", "Number of channels used in common mode calculation after cuts;Number of channels;Events", 256, -0.5, 255.5);

  commModeGrSlope = new TGraph*[nChips];
  commModeGrOffset =  new TGraph*[nChips];
  commModeSlopeDistr = new TH1F*[nChips];
  commModeOffsetDistr = new TH1F*[nChips];

  for(int iChip = 0; iChip < nChips; ++iChip){
    sprintf(name, "commModeGrSlopeChip%d", iChip);
    sprintf(title, "Slope of the common mode vs. event chip %d", iChip);
    commModeGrSlope[iChip] = new TGraph();
    commModeGrSlope[iChip]->SetName(name);
    commModeGrSlope[iChip]->SetTitle(title);

    sprintf(name, "commModeGrOffsetChip%d", iChip);
    sprintf(title, "Offset of the common mode vs. event chip %d", iChip);
    commModeGrOffset[iChip] = new TGraph();
    commModeGrOffset[iChip]->SetName(name);
    commModeGrOffset[iChip]->SetTitle(title);

    sprintf(name, "commModeSlopeDistrChip%d", iChip);
    sprintf(title, "Common mode slope distribution chip %d;Slope [ADC / Ch.];Entries", iChip);
    commModeSlopeDistr[iChip] = new TH1F(name, title, 200, -1, 1);

    sprintf(name, "commModeOffsetDistrChip%d", iChip);
    sprintf(title, "Common mode offset distribution chip %d;Slope [ADC / Ch.];Entries", iChip);
    commModeOffsetDistr[iChip] = new TH1F(name, title, 1000, -500, 500);
  }

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
  std::ifstream pedStr;
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
  std::ifstream calStr;
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

 void DataRun::CommonModeCalculation(double* phChannels, int chipNum)
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

  std::vector<int> chInUse;
  // select the channels for the chip in consideration
  for(unsigned int i = 0; i < goodChannels.size(); ++i)
    if(goodChannels[i] < nChChip * (chipNum + 1) && goodChannels[i] >= nChChip * chipNum)
      chInUse.push_back(goodChannels[i]);

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

      if(S < 2){ // not enough channels for the calculation
	commMode[chipNum][0] = 0;
	commMode[chipNum][1] = 0;
	return;
      }

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

  commMode[chipNum][0] = a;
  commMode[chipNum][1] = b;

  chInCommMode->Fill(chInUse.size());

  return;
}

void DataRun::doSpecificStuff()
{
  double pedSubPH[nChannels] = {0};
  for(int i = 0; i < nChannels; ++i) // reset to 0 the signal
    {
      signal[i] = 0;
      caliSignal[i] = 0;
    }

  for(unsigned int iCh = 0; iCh < goodChannels.size(); ++iCh) // pedestal subtraction
    pedSubPH[goodChannels[iCh]] = adcPH[goodChannels[iCh]] - pedestals[goodChannels[iCh]];

  for(int iChip = 0; iChip < nChips; ++iChip)
    CommonModeCalculation(pedSubPH, iChip);

  int evtNum = commModeGrSlope[0]->GetN();
  for(int iChip = 0; iChip < nChips; ++iChip){
    commModeGrSlope[iChip]->SetPoint(evtNum, evtNum, commMode[iChip][1]);
    commModeGrOffset[iChip]->SetPoint(evtNum, evtNum, commMode[iChip][0]);
    commModeSlopeDistr[iChip]->Fill(commMode[iChip][1]);
    commModeOffsetDistr[iChip]->Fill(commMode[iChip][0]);
  }

  for(int iChip = 0; iChip < nChips; ++iChip){
    for(unsigned int iCh = 0; iCh < goodChannels.size(); ++iCh) // common mode subtraction
      	  if(goodChannels[iCh] < nChChip * (iChip + 1) && goodChannels[iCh] >= nChChip * iChip) // select the channels for the right chip to have the right common mode
	    signal[goodChannels[iCh]] = pedSubPH[goodChannels[iCh]] - commMode[iChip][0] - commMode[iChip][1] * goodChannels[iCh];
  }
  
  FindClusters(signal);

  for(unsigned int iCh = 0; iCh < goodChannels.size(); ++iCh) // apply calibration to the signal
    {
      toAliE->SetParameter(0, fabs(calibrations[goodChannels[iCh]][1])); // use only the gain

      caliSignal[goodChannels[iCh]] = toAliE->Eval(signal[goodChannels[iCh]]);
    }

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

      // std::cout << "------------------- Event " << i << '\n';
      // std::cout << "N clusters: " << nClust << std::endl;
      // if(nClust != 0)
      // 	for(int iCl = 0; iCl < nClust; iCl++)
      // 	  {
      // 	    clu = cluVecPtr->at(iCl);
      // 	    std::cout << "Cluster " << iCl << ": ";
      // 	    for(unsigned int iStr = 0; iStr < clu.strips.size(); ++iStr)
      // 	      std::cout << ' ' << clu.strips.at(iStr);
      // 	    std::cout << std::endl;
      // 	  }
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

  for(int iChip = 0; iChip < nChips; iChip++){
    commModeGrSlope[iChip]->Draw("AP");
    commModeGrSlope[iChip]->GetXaxis()->SetTitle("Event");
    commModeGrSlope[iChip]->GetYaxis()->SetTitle("Slope [ADC / Ch. number]");
    commModeGrSlope[iChip]->Write();
    
    commModeGrOffset[iChip]->Draw("AP");
    commModeGrOffset[iChip]->GetXaxis()->SetTitle("Event");
    commModeGrOffset[iChip]->GetYaxis()->SetTitle("Offset [ADC]");
    commModeGrOffset[iChip]->Write();
  }

  delete serv;

  clusterSize->Write();

  for(int iChip = 0; iChip < nChips; iChip++){
    commModeSlopeDistr[iChip]->Write();
    commModeOffsetDistr[iChip]->Write();
  }

  return;
}

void DataRun::AddStrip(cluster* clu, Float_t* phArray, int stripNum)
{
  clu->adcTot += phArray[stripNum] * scaleFactor; // the total charge is the sum with sign
  clu->strips.push_back(stripNum);
  clu->adcStrips.push_back(phArray[stripNum] * scaleFactor);

  toAliE->SetParameter(0, fabs(calibrations[stripNum][1])); // use only the gain
  
  double charge = toAliE->Eval(phArray[stripNum]);
  clu->qTot += charge;
  clu->qStrips.push_back(charge);

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

      if(phChannels[chNum - 1] == 0 || phChannels[chNum + 1] == 0) continue; // the strips confining bad channels can not be seeds

      if(phChannels[chNum] / fabs(phChannels[chNum]) == polarity &&
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
		if(phChannels[chNum] / fabs(phChannels[chNum]) == polarity && // right polarity also imposed for neighbors
		   fabs(phChannels[chNum] / noise[chNum]) > snrNeigh && usedStrip[chNum] == false) // check for snr and that the strip has not already been used
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
		if(phChannels[chNum] / fabs(phChannels[chNum]) == polarity && // right polarity also imposed for neighbors
		   fabs(phChannels[chNum] / noise[chNum]) > snrNeigh && usedStrip[chNum] == false) // check for snr and that the strip has not already been used
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
  double sumPosADC = 0;
  double sumADC = 0;

  double sumPosQ = 0;
  double sumQ = 0;

  for(unsigned int iStr = 0; iStr < clu->strips.size(); ++iStr)
    {
      sumPosADC += clu->strips.at(iStr) * fabs(clu->adcStrips.at(iStr));
      sumADC += fabs(clu->adcStrips.at(iStr));

      sumPosQ += clu->strips.at(iStr) * fabs(clu->qStrips.at(iStr));
      sumQ += fabs(clu->qStrips.at(iStr));
    }

  clu->posStrAdc = sumPosADC / sumADC;
  clu->posmmAdc = clu->posStrAdc * pitch + 0.5 * pitch;

  clu->posStrQ = sumPosQ / sumQ;
  clu->posmmQ = clu->posStrQ * pitch + 0.5 * pitch;

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
