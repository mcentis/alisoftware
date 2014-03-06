void genLangaus()
{
  TF1* lan = new TF1("lan", "landau", -10, 150);
  lan->SetParameters(100, 40, 2); // parameters: constant (as for gaus), mpv, width

  TH1F* lanHist = new TH1F("lanHist", "Generated Landau distribution;Energy deposit [A.U.];Entries", 160, -10, 150);

  TF1* gaus = new TF1("gaus", "gaus", -20, 20);
  gaus->SetParameters(100, 0, 3);

  TH1F* gausHist = new TH1F("gausHist", "Generated Gauss distribution;Noise [A.U.];Entries", 40, -20, 20);

  TH1F* lanGausHist = new TH1F("lanGausHist", "Convolution of Landau and Gaussian distributions;Measured energy deposit [A.U.];Entries", 160, -10, 150);

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
