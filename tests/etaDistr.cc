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

void etaDistr()
{
  return;
}

void etaDistr(const char* inFile, double timeCut1 = 0, double timeCut2 = 115)
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

  TH1D* etaDistr = new TH1D("etaDistr", "#eta distribution;#eta;Entries", 120, -0.5, 1.5);

  long int nEntries = cooked->GetEntries();
  int nClust;

  // left and right strip charge
  double sL;
  double sR;

  for(long int i = 0; i < nEntries; i++)
    {
      cooked->GetEntry(i);
      raw->GetEntry(i);

      if(time <= timeCut1 || time >= timeCut2) continue; // select the events in the time cut

      nClust = cluVecPtr->size();

      for(int iCl = 0; iCl < nClust; ++iCl)
	{
	  clu = cluVecPtr->at(iCl);

	  if(clu.strips.size() != 2) continue; // select 2 strips clusters

	  std::cout << clu.strips.size() << std::endl;

	  //determination of left and right strip
	  if(clu.strips.at(0) < clu.strips.at(1))
	    {
	      sL = clu.adcStrips.at(0);
	      sR = clu.adcStrips.at(1);
	    }
	  else
	    {
	      sR = clu.adcStrips.at(0);
	      sL = clu.adcStrips.at(1);
	    }

	  sR = fabs(sR);
	  sL = fabs(sL);

	  etaDistr->Fill(sR / (sR + sL));
	}
    }

  TCanvas* etaCan = new TCanvas("etaCan");
  etaCan->SetTitle("Eta distribution");
  etaDistr->Draw();

  return;
}
