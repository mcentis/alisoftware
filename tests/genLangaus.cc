void genLangaus()
{
  TF1* lan = new TF1("lan", "landau", 0, 150);
  lan->SetParameters(1, 40, 2);

  TH1F* lanHist = new TH1F("lanHist", "Generated Landau distribution;Energy deposit [A.U.];Entries", 150, 0, 150);

  TF1* gaus = new TF1("gaus", "gaus", -20, 20);
  gaus->SetParameters(100, 0, 3);

  TH1F* gausHist = new TH1F("gausHist", "Generated Gauss distribution;Noise [A.U.];Entries", 30, -15, 15);

  TH1F* lanGausHist = new TH1F("lanGausHist", "Convolution of Landau and Gaussian distributions (hopefully);Measured energy deposit [A.U.];Entries", 150, 0, 150);

  float lanRnd;
  float gausRnd;

  for(int i = 0; i < 100000; i++)
    {
      lanRnd = lan->GetRandom();
      lanHist->Fill(lanRnd);

      gausRnd = gaus->GetRandom();
      gausHist->Fill(gausRnd);

      lanGausHist->Fill(lanRnd + gausRnd);
    }

  TCanvas* lanCan = new TCanvas("lanCan");
  lanCan->SetTitle("Generated Landau");
  lanHist->Draw();

  TCanvas* gausCan = new TCanvas("gausCan");
  gausCan->SetTitle("Generated Gaussian");
  gausHist->Draw();

  TCanvas* lanGausCan = new TCanvas("lanGausCan");
  lanGausCan->SetTitle("Landau Gaussian convolution");
  lanGausHist->Draw();
  return;
}
