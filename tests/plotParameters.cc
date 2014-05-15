void plotParameters(const char* fileName)
{
  const int nParameters = 2;

  TFile* inFile = TFile::Open(fileName);
  TDirectory* dirPos = (TDirectory*) inFile->Get("Graphs_posCal_Parameters");
  TDirectory* dirNeg = (TDirectory*) inFile->Get("Graphs_negCal_Parameters");

  TGraph* posParGr[nParameters];
  TGraph* negParGr[nParameters];

  char name[50];
  char title[200];

  for(int i = 0; i < nParameters; ++i)
    {
      sprintf(name, "par_%i_vsChPosCal", i);
      posParGr[i] = (TGraph*) dirPos->Get(name);
      posParGr[i]->SetFillColor(kWhite);
      posParGr[i]->SetLineColor(kRed);

      sprintf(name, "par_%i_vsChNegCal", i);
      negParGr[i] = (TGraph*) dirNeg->Get(name);
      negParGr[i]->SetFillColor(kWhite);
      negParGr[i]->SetLineColor(kBlue);
    }

  TMultiGraph* multiPosNeg[nParameters];

  for(int i = 0; i < nParameters; ++i)
    {
      sprintf(name, "muliPosNegPar_%i", i);
      sprintf(title, "Parameter %i positive and negative calibration;Par %i;Entries", i);

      multiPosNeg[i] = new TMultiGraph(name, title);
    }

  TCanvas* grCan[nParameters];
  TLegend* leg;

  for(int i = 0; i < nParameters; ++i)
    {
      multiPosNeg[i]->Add(posParGr[i]);
      multiPosNeg[i]->Add(negParGr[i]);

      sprintf(name, "canGrPar_%i", i);
      sprintf(title, "Parameter %i graph", i);

      grCan[i] = new TCanvas(name, title);
      grCan[i]->cd();
      multiPosNeg[i]->Draw("APL");
      leg = grCan[i]->BuildLegend();
      leg->SetFillColor(kWhite);
    }

  return;
}
