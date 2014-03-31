#include "vector"
#include "../include/clusterDef.h"
#include "../include/constants.h"

#ifdef __CINT__
#pragma link C++ class cluster+;
#pragma link C++ class vector<cluster>+;
#endif

#include "TFile.h"
#include "TTree.h"
#include "TLegend.h"
#include "TStyle.h"
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

void etaDistr(const char* inFile,double timeCut1 = 0, double timeCut2 = 115,  int polarity = 1, const char* borderStripsFile = "")
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

  polarity = polarity / abs(polarity); // make the polarity a unit with sign

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
  Float_t signal[nChannels];

  cooked->SetBranchAddress("clustVec", &cluVecPtr);
  cooked->SetBranchAddress("signal", signal);
  raw->SetBranchAddress("time", &time);

  // deactivate all branches
  cooked->SetBranchStatus("*", 0);
  raw->SetBranchStatus("*", 0);
  //activate the interesting ones
  cooked->SetBranchStatus("clustVec*", 1);
  cooked->SetBranchStatus("signal", 1);
  raw->SetBranchStatus("time", 1);

  TH1D* etaDis = new TH1D("etaDis", "#eta distribution;#eta;Entries", 120, -0.5, 1.5);

  TH2D* clusterShapePH = new TH2D("clusterShapePH", "Pulse heigth vs. position in the cluster (seed strip in 0);Strip;Signal [ADC];Entries", 21, -10.5, 10.5, 400, -200, 200);

  TH1D* seed = new TH1D("seed", "Seed strip (highest |charge| in the cluster);Charge [ADC];Entries", 400, -200, 200);
  TH1D* d1L = new TH1D("d1L", "Left neighbour of the seed strip (if included in the cluster);Charge [ADC];Entries", 400, -200, 200);
  TH1D* d1R = new TH1D("d1R", "Right neighbour of the seed strip (if included in the cluster);Charge [ADC];Entries", 400, -200, 200);

  TH1D* leftRatio = new TH1D("leftRatio", "Signal from the left neighbour of the seed strip divided by the seed signal;Charge ratio;Entries", 100, -1.25, 1.25);
  TH1D* rightRatio = new TH1D("rightRatio", "Signal from the right neighbour of the seed strip divided by the seed signal;Charge ratio;Entries", 100, -1.25, 1.25);

  TH2D* diffVsSeed = new TH2D("diffVsSeed", "PH(L) - PH(R) vs PH(Seed), 2 strips clusters;PH(Seed) [ADC];PH(L) - PH(R) [ADC]", 400, -200, 200, 200, -100, 100);

  TH1D* neighNoiseL = new TH1D("neighNoiseL", "Left neighbour of a single strip cluster;Charge [ADC];Entries", 100, -50, 50);
  TH1D* neighNoiseR = new TH1D("neighNoiseR", "Right neighbour of a single strip cluster;Charge [ADC];Entries", 100, -50, 50);

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

	  if(clu.strips.size() == 1) // one strip clusters
	    if(signal[seedNum + 1] && signal[seedNum - 1]) // exclude bad channels
	      {
		neighNoiseL->Fill(signal[seedNum - 1]);
		neighNoiseR->Fill(signal[seedNum + 1]);
	      }
	    
	  seed->Fill(seedPH); // fill seed charge graph

	  for(unsigned int iSt = 0; iSt < clu.strips.size(); ++iSt) // fill the graphs
	    {
	      if(clu.strips.at(iSt) == (seedNum - 1)) // left first neighbour
		{
		  d1L->Fill(clu.adcStrips.at(iSt));
		  leftRatio->Fill(clu.adcStrips.at(iSt) / seedPH);
		}

	      if(clu.strips.at(iSt) == (seedNum + 1)) // right first neighbour
		{
		  d1R->Fill(clu.adcStrips.at(iSt));
		  rightRatio->Fill(clu.adcStrips.at(iSt) / seedPH);
		}

	      clusterShapePH->Fill(clu.strips.at(iSt) - seedNum, clu.adcStrips.at(iSt));
	    }

	  // eta defined just by the seed strip, the neighbor with bigger charge belongs to the pair
	  if(signal[seedNum + 1] && signal[seedNum - 1]) // exclude bad channels
	    if(signal[seedNum + 1] / fabs(signal[seedNum + 1]) == polarity && signal[seedNum - 1] / fabs(signal[seedNum - 1]) == polarity) // exclude channels with a polarity different that expected
	      {
		if(fabs(signal[seedNum + 1]) > fabs(signal[seedNum - 1]))
		  {
		    sL = seedPH;
		    sR = signal[seedNum + 1];
		  }
		else
		  {
		    sR = seedPH;
		    sL = signal[seedNum - 1];
		  }
		etaDis->Fill(sR / (sR + sL));
		diffVsSeed->Fill(seedPH, sL - sR);
	      }

	  // if(clu.strips.size() != 2) continue; // select 2 strips clusters to determine eta

	  // //determination of left and right strip
	  // if(clu.strips.at(0) < clu.strips.at(1))
	  //   {
	  //     sL = clu.adcStrips.at(0);
	  //     sR = clu.adcStrips.at(1);
	  //   }
	  // else
	  //   if(clu.strips.at(0) > clu.strips.at(1))
	  //     {
	  // 	sR = clu.adcStrips.at(0);
	  // 	sL = clu.adcStrips.at(1);
	  //     }
	  //   else
	  //     {
	  // 	std::cout << "Error!! Strip numbers: " << clu.strips.at(0) << " , "  << clu.strips.at(1) << "   Total strips in the cluster: " << clu.strips.size() << std::endl;
	  // 	continue;
	  //     }

	  // diffVsSeed->Fill(seedPH, sL - sR);
	}
    }

  TProfile* clusterPHprof = clusterShapePH->ProfileX("clusterPHprof");
  clusterPHprof->SetTitle("Mean signal in the different cluster strips (seed in 0)");

  TCanvas* etaCan = new TCanvas("etaCan");
  etaCan->SetTitle("Eta distribution");
  etaDis->Draw();

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

  // ratio of the histograms
  leftRatio->Sumw2(); // important to add and divide histos
  leftRatio->SetLineColor(kRed);
  leftRatio->SetLineWidth(2);

  rightRatio->Sumw2(); // important to add and divide histos
  rightRatio->SetLineColor(kBlue);
  rightRatio->SetLineWidth(2);

  // normalize the histos to the same value
  rightRatio->Scale(1 / rightRatio->GetEntries());
  leftRatio->Scale(1 / leftRatio->GetEntries());

  TH1D* ratioRatio = new TH1D(*leftRatio); // ratio of the ratioes
  ratioRatio->SetName("ratioRatio");
  ratioRatio->SetTitle(";PH(Neigh) / PH(Seed);leftRatio / rightRatio");
  ratioRatio->Divide(rightRatio);
  ratioRatio->SetLineWidth(2);
  ratioRatio->SetLineColor(kViolet);
  ratioRatio->GetYaxis()->SetRangeUser(-1, 3);

  leftRatio->SetTitle(";;Entries");
  rightRatio->SetTitle(";;Entries");

  // set these to have fancier results
  // gStyle->SetOptStat(0);
  // gStyle->SetOptTitle(0);

  TLegend* leg = new TLegend(0.2, 0.6, 0.4, 0.8);
  leg->AddEntry(leftRatio, "PH(L) / PH(Seed)");
  leg->AddEntry(rightRatio, "PH(R) / PH(Seed)");
  leg->SetFillColor(kWhite);

  TCanvas* crTlkCan = new TCanvas("crTlkCan");
  crTlkCan->SetTitle("Cross talk");
  crTlkCan->Divide(1, 2, 0, 0);
  crTlkCan->cd(1);
  crTlkCan->GetPad(1)->SetRightMargin(0.1);
  rightRatio->Draw("HISTO");
  leftRatio->Draw("HISTOSAME");
  leg->Draw();
  crTlkCan->cd(2);
  crTlkCan->GetPad(2)->SetRightMargin(0.1);
  crTlkCan->GetPad(2)->SetGridy();
  ratioRatio->Draw();

  TCanvas* diffSeedCan = new TCanvas("diffSeedCan");
  diffSeedCan->SetTitle("Difference vs seed");
  diffVsSeed->Draw("COLZ");

  TCanvas* neiNoCan = new TCanvas("neiNoCan");
  neiNoCan->SetTitle("Noise on neighbours");
  neiNoCan->Divide(1, 2);
  neiNoCan->cd(1);
  neighNoiseL->Draw();
  neiNoCan->cd(2);
  neighNoiseR->Draw();

  return;
}
