#include "vector"
#include "../include/clusterDef.h"
#include "../include/constants.h"

#ifdef __CINT__
#pragma link C++ class cluster+;
#pragma link C++ class vector<cluster>+;
#endif

#include "TFile.h"
#include "TTree.h"
#include "TGraph.h"
#include "TH2D.h"
#include "TProfile.h"
#include "TCanvas.h"
#include "TLine.h"

#include "iostream"

void etaDistr()
{
  return;
}

void etaDistr(const char* inFile,double timeCut1 = 0, double timeCut2 = 115,  const char* borderStripsFile = "")
{
  TFile* file = TFile::Open(inFile, "READ");

  if(!file)
    {
      std::cout << "Unable to open " << file << std::endl;
      return;
    }

  //read the border strips
  TGraph* bordStr = NULL;
  double* bordList = NULL;
  if(borderStripsFile[0] != '\0') // check if there is a file
    {
      bordStr = new TGraph(borderStripsFile);
      bordList = bordStr->GetX();
      TCanvas* brdCan = new TCanvas("brdCan");
      brdCan->SetTitle("Border strips");
      bordStr->Draw("A*");
    }

  bool isBorder[nChannels] = {0}; // true if the strip is a border strip
  if(bordList)
    for(int i = 0; i < bordStr->GetN(); ++i) isBorder[(int) bordList[i]] = true;

  //for(int i = 0; i < nChannels; ++i) std::cout << "ch: " << i << " is border: " << isBorder[i] << std::endl;

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

  TH1D* etaDis = new TH1D("etaDis", "#eta distribution;#eta;Entries", 120, -0.5, 1.5);

  TH2D* clusterShapePH = new TH2D("clusterShapePH", "Pulse heigth vs. position in the cluster (seed strip in 0);Strip;Signal [ADC];Entries", 21, -10.5, 10.5, 400, -200, 200);

  TH1D* seed = new TH1D("seed", "Seed strip (highest |charge| in the cluster);Charge [ADC];Entries", 400, -200, 200);
  TH1D* d1L = new TH1D("d1L", "Left neighbour of the seed strip (if included in the cluster);Charge [ADC];Entries", 400, -200, 200);
  TH1D* d1R = new TH1D("d1R", "Right neighbour of the seed strip (if included in the cluster);Charge [ADC];Entries", 400, -200, 200);

  long int nEntries = cooked->GetEntries();
  int nClust;

  // left and right strip charge
  double sL;
  double sR;

  double seedPH = -1; // signal of the seed strip
  int seedNum = -1; // number of the seed strip

  for(long int i = 0; i < nEntries; i++)
    {
      cooked->GetEntry(i);
      raw->GetEntry(i);

      if(time <= timeCut1 || time >= timeCut2) continue; // select the events in the time cut

      nClust = cluVecPtr->size();

      for(int iCl = 0; iCl < nClust; ++iCl)
	{
	  clu = cluVecPtr->at(iCl);

	  // find the strip with the biggest ph in the cluster
	  seedPH = 0;
	  for(unsigned int iSt = 0; iSt < clu.strips.size(); ++iSt)
	    if(fabs(clu.adcStrips.at(iSt)) > fabs(seedPH))
	      {
	  	seedPH = clu.adcStrips.at(iSt);
	  	seedNum = clu.strips.at(iSt);
	      }

	  if(isBorder[seedNum]) continue; // if the seed is a border strip
	    
	  seed->Fill(seedPH); // fill seed charge graph

	  for(unsigned int iSt = 0; iSt < clu.strips.size(); ++iSt) // fill the graphs
	    {
	      if(clu.strips.at(iSt) == (seedNum - 1)) d1L->Fill(clu.adcStrips.at(iSt)); // left first neighbour
	      if(clu.strips.at(iSt) == (seedNum + 1)) d1R->Fill(clu.adcStrips.at(iSt)); // right first neighbour
	      clusterShapePH->Fill(clu.strips.at(iSt) - seedNum, clu.adcStrips.at(iSt));
	    }

	  if(clu.strips.size() != 2) continue; // select 2 strips clusters to determine eta

	  //determination of left and right strip
	  if(clu.strips.at(0) < clu.strips.at(1))
	    {
	      sL = clu.adcStrips.at(0);
	      sR = clu.adcStrips.at(1);
	    }
	  else
	    if(clu.strips.at(0) > clu.strips.at(1))
	      {
		sR = clu.adcStrips.at(0);
		sL = clu.adcStrips.at(1);
	      }
	    else
	      {
		std::cout << "Error!! Strip numbers: " << clu.strips.at(0) << " , "  << clu.strips.at(1) << "   Total strips in the cluster: " << clu.strips.size() << std::endl;
		continue;
	      }

	  etaDis->Fill(sR / (sR + sL));
	}
    }

  TProfile* clusterPHprof = clusterShapePH->ProfileX("clusterPHprof");
  clusterPHprof->SetTitle("Mean signal in the different cluster strips (seed in 0)");

  TCanvas* etaCan = new TCanvas("etaCan");
  etaCan->SetTitle("Eta distribution");
  etaDis->Draw("E");

  TCanvas* cluShapeCan = new TCanvas("cluShapePH");
  cluShapeCan->SetTitle("Cluster shape signal");
  cluShapeCan->Divide(2, 0);
  cluShapeCan->cd(1);
  clusterShapePH->Draw("COLZ");
  cluShapeCan->cd(2);
  clusterPHprof->Draw();

  TCanvas* striCan = new TCanvas("striCan");
  striCan->SetTitle("Strip signal");
  striCan->Divide(3, 0);
  striCan->cd(1);
  seed->Draw();
  striCan->cd(2);
  d1L->Draw();
  striCan->cd(3);
  d1R->Draw();

  return;
}
