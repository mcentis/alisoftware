void meanSlopeCMCorrelation(const char* file1_cmMean, const char* file2_cmSlope)
{
  TFile* f1 = TFile::Open(file1_cmMean);
  TTree* cook1 = f1->Get("cookedEvtTree");
  Float_t commM1;
  cook1->SetBranchStatus("*", 0);
  cook1->SetBranchStatus("commMode", 1);
  cook1->SetBranchAddress("commMode", &commM1);

  TFile* f2 = TFile::Open(file2_cmSlope);
  TTree* cook2 = f2->Get("cookedEvtTree");
  Float_t commM2[2];
  cook2->SetBranchStatus("*", 0);
  cook2->SetBranchStatus("commMode", 1);
  cook2->SetBranchAddress("commMode", commM2);

  TH2F* slopeCorr = new TH2F("slopeCorr", "Correlation of the common mode slope;Mean [ADC];Slope [ADC / Ch.]", 1000, -500, 500, 200, -1, 1);

  int nEntries = cook1->GetEntries();

  double sum1 = 0;
  double sum2 = 0;

  for(int i = 0; i < nEntries; ++i)
    {
      cook1->GetEntry(i);
      cook2->GetEntry(i);

      sum1 += commM1;
      sum2 += commM2[1];
    }

  double meanSlope1 = sum1 / nEntries;
  double meanSlope2 = sum2 / nEntries;

  double sumProd = 0;
  double sumDevSq1 = 0;
  double sumDevSq2 = 0;

  double dev1;
  double dev2;

  for(int i = 0; i < nEntries; ++i)
    {
      cook1->GetEntry(i);
      cook2->GetEntry(i);

      dev1 = commM1 - meanSlope1;
      dev2 = commM2[1] - meanSlope2;

      sumProd += dev1 * dev2;
      sumDevSq1 += dev1 * dev1;
      sumDevSq2 += dev2 * dev2;

      slopeCorr->Fill(commM1, commM2[1]);
    }

  TCanvas* corrHist = new TCanvas("corrHist", "histo");
  slopeCorr->Draw("colz");

  TProfile* pro = slopeCorr->ProfileX("profilex");

  TCanvas* corrPro = new TCanvas("corrPro", "profile");
  pro->Draw();

  std::cout << "Correlation coefficient: " << sumProd / (sqrt(sumDevSq1) * sqrt(sumDevSq2)) << std::endl;

  return;
}
