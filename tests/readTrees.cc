
#include "vector"
#include "../include/clusterDef.h"

#ifdef __CINT__
#pragma link C++ class cluster+;
#pragma link C++ class vector<cluster>+;
#endif

#include "TFile.h"
#include "TTree.h"
#include "TH2D.h"
#include "TProfile.h"
#include "TCanvas.h"
#include "TLine.h"

#include "iostream"

void readTrees()
{
  return;
}

void readTrees(const char* inFile, double timeCut1 = 0, double timeCut2 = 115)
{
  TFile* file = TFile::Open(inFile, "READ");

  if(!file)
    {
      std::cout << "Unable to open " << file << std::endl;
      return;
    }

  TTree* raw = (TTree*) file->Get("rawEvtTree");
  TTree* cooked = (TTree*) file->Get("cookedEvtTree");

  std::cout << "Raw tree events: " << raw->GetEntries() << std::endl;
  std::cout << "Cooked tree events: " << cooked->GetEntries() << std::endl;

  std::vector<cluster>* cluVecPtr = NULL;

  cluster clu;
  Float_t time;

  cooked->SetBranchAddress("clustVec", &cluVecPtr);
  raw->SetBranchAddress("time", &time);

  // deactivate all branches
  cooked->SetBranchStatus("*", 0);
  raw->SetBranchStatus("*", 0);
  //activate the interesting ones
  cooked->SetBranchStatus("clustVec*", 1);
  raw->SetBranchStatus("time", 1);

  TH2D* signalTime = new TH2D("signalTime", "Cluster charge vs time;Time [ns];Cluster charge [ADC]", 60, 0, 120, 1024, -511.5, 511.5);
  TH1D* deposit = new TH1D("deposit", "Energy deposit;Cluster charge [ADC]", 1024, -0.5, 1023.5);

  long int nEntries = cooked->GetEntries();
  int nClust;

  TH1D* posStrAdc = new TH1D("posStrAdc", "Cluster position in strip number;Position [Strip number];Entries", 1000, 0, 256);
  TH1D* posmmAdc = new TH1D("posmmAdc", "Cluster position in mm;Position [mm];Entries", 1500, 0, 30);
  TH1I* clustSize = new TH1I("clustSize", "Cluster size in the time cut;Size [Stips];Entries", 21, -0.5, 20.5);

  for(long int i = 0; i < nEntries; i++)
    {
      cooked->GetEntry(i);
      raw->GetEntry(i);

      nClust = cluVecPtr->size();

      for(int iCl = 0; iCl < nClust; ++iCl)
	{
	  clu = cluVecPtr->at(iCl);
	  //	  std::cout << clu.adcTot << std::endl;
	  signalTime->Fill(time, clu.adcTot);
	  if(timeCut1 < time && time < timeCut2)
	    {
	      deposit->Fill(fabs(clu.adcTot));
	      clustSize->Fill(clu.strips.size());
	    }

	  posStrAdc->Fill(clu.posStrAdc);
	  posmmAdc->Fill(clu.posmmAdc);
	}
    }

  TProfile* timeProfile = signalTime->ProfileX("timeProfile");
  timeProfile->SetTitle("Time profile of the cluster charge;Time [ns];Signal [ADC]");

  double ymin = -50; //timeProfile->GetYaxis()->GetXmin();
  double ymax = 50; //timeProfile->GetYaxis()->GetXmax();

  TLine* cut1 = new TLine(timeCut1, ymin, timeCut1, ymax);
  cut1->SetLineColor(kRed);

  TLine* cut2 = new TLine(timeCut2, ymin, timeCut2, ymax);
  cut2->SetLineColor(kRed);

  TCanvas* can = new TCanvas("Signal time");
  can->SetTitle("Signal time");
  can->Divide(2, 1);

  can->cd(1);
  signalTime->Draw("COLZ");
  //timeProfile->Draw("SAME");

  can->cd(2);
  timeProfile->Draw();

  can->Update();
  can->cd(2);
  cut1->Draw("SAME");
  cut2->Draw("SAME");

  TCanvas* can2 = new TCanvas("Energy deposit");
  can2->SetTitle("Energy deposit");
  can2->cd();
  deposit->Draw();

  TCanvas* can3 = new TCanvas("pos");
  can3->SetTitle("Position");
  can3->Divide(2, 1);

  can3->cd(1);
  posStrAdc->Draw();

  can3->cd(2);
  posmmAdc->Draw();

  TCanvas* can4 = new TCanvas("cluSize");
  can4->SetTitle("Cluster Size");
  clustSize->Draw();

  return;
}
