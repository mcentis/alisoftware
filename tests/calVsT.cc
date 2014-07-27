void calVsT()
{
  const char* baseDir = "/space/centis/alisoft/alisoftware/data/M200Punirrad/filesCalVsT/"; // it is important to use the final /
  // runs and temperatures for delay 78
  const int nPoints = 6;
  double temp[nPoints] = {13, 14, 19, 22, 24, 24};
  char* runs[nPoints] = {"run000017.root", "run000013.root", "run000010.root", "run000007.root", "run000004.root", "run000002.root"};

  // runs and temperatures for delay 82
  // const int nPoints = 4;
  // double temp[nPoints] = {13, 14, 19, 22};
  // char* runs[nPoints] = {"run000016.root", "run000012.root", "run000009.root", "run000006.root"};

  TGraphErrors* slopePos = new TGraphErrors();
  slopePos->SetName("slopePos");
  slopePos->SetTitle("slope pos cal, del  ns");

  TGraphErrors* offsetPos = new TGraphErrors();
  offsetPos->SetName("offsetPos");
  offsetPos->SetTitle("offset pos cal, del  ns");

  TGraphErrors* slopeNeg = new TGraphErrors();
  slopeNeg->SetName("slopeNeg");
  slopeNeg->SetTitle("slope neg cal, del  ns");

  TGraphErrors* offsetNeg = new TGraphErrors();
  offsetNeg->SetName("offsetNeg");
  offsetNeg->SetTitle("offset neg cal, del  ns");

  TFile* inFile;
  TDirectory* dir;
  char fileName[200];
  TH1* hist;

  double slope, offset;
  double eOff, eSlope, err; //errors
  const double charge = 7500;
  TGraphErrors* respPos = new TGraphErrors();
  respPos->SetName("respPos");
  respPos->SetTitle("calculated adc for pos 7500 e pulse");

  TGraphErrors* respNeg = new TGraphErrors();
  respNeg->SetName("respNeg");
  respNeg->SetTitle("calculated adc for neg 7500 e pulse");

  std::cout << "created stuff" << std::endl;

  for(int i = 0; i < nPoints; ++i)
    {
      sprintf(fileName, "%s%s", baseDir, runs[i]);
      inFile = TFile::Open(fileName);

      // std::cout << fileName << std::endl;
      if(inFile == 0) continue;

      std::cout << "--------------- chip temperature " << temp[i] << std::endl;

      dir = (TDirectory*) inFile->Get("Graphs_posCal_Parameters");
      hist = (TH1*) dir->Get("distrPar_1_posCal_goodCh");
      slopePos->SetPoint(i, temp[i], hist->GetMean());
      slopePos->SetPointError(i, 0, hist->GetRMS());
      slope = hist->GetMean();
      eSlope = hist->GetRMS();

      hist = (TH1*) dir->Get("distrPar_0_posCal_goodCh");
      offsetPos->SetPoint(i, temp[i], hist->GetMean());
      offsetPos->SetPointError(i, 0, hist->GetRMS());
      offset = hist->GetMean();
      eOff = hist->GetRMS();

      respPos->SetPoint(i, temp[i], charge * slope + offset);
      err = sqrt(charge * charge * eSlope * eSlope + eOff * eOff);
      respPos->SetPointError(i, 0, err);

      std::cout << "pos cal slope " << slope << " offset " << offset << std::endl;

      dir = (TDirectory*) inFile->Get("Graphs_negCal_Parameters");
      hist = (TH1*) dir->Get("distrPar_1_negCal_goodCh");
      slopeNeg->SetPoint(i, temp[i], hist->GetMean());
      slopeNeg->SetPointError(i, 0, hist->GetRMS());
      slope = hist->GetMean();
      eSlope = hist->GetRMS();

      hist = (TH1*) dir->Get("distrPar_0_negCal_goodCh");
      offsetNeg->SetPoint(i, temp[i], hist->GetMean());
      offsetNeg->SetPointError(i, 0, hist->GetRMS());
      offset = hist->GetMean();
      eOff = hist->GetRMS();

      respNeg->SetPoint(i, temp[i], charge * slope + offset);
      err = sqrt(charge * charge * eSlope * eSlope + eOff * eOff);
      respNeg->SetPointError(i, 0, err);

      std::cout << "neg cal slope " << slope << " offset " << offset << std::endl;

      inFile->Close();
    }

  std::cout << "readen files" << std::endl;

  TCanvas* posCan = new TCanvas("posCan", "posCan");
  posCan->Divide(2, 1);
  posCan->cd(1);
  slopePos->Draw("AP");
  slopePos->GetXaxis()->SetTitle("Chip temperature [C]");
  slopePos->GetYaxis()->SetTitle("Slope fit [ADC / e]");
  posCan->cd(2);
  offsetPos->Draw("AP");
  offsetPos->GetXaxis()->SetTitle("Chip temperature [C]");
  offsetPos->GetYaxis()->SetTitle("Offset fit [ADC]");
  posCan->Modified();
  posCan->Update();

  TCanvas* negCan = new TCanvas("negCan", "negCan");
  negCan->Divide(2, 1);
  negCan->cd(1);
  slopeNeg->Draw("AP");
  slopeNeg->GetXaxis()->SetTitle("Chip temperature [C]");
  slopeNeg->GetYaxis()->SetTitle("Slope fit [ADC / e]");
  negCan->cd(2);
  offsetNeg->Draw("AP");
  offsetNeg->GetXaxis()->SetTitle("Chip temperature [C]");
  offsetNeg->GetYaxis()->SetTitle("Offset fit [ADC]");
  negCan->Modified();
  negCan->Update();

  TCanvas* respCan = new TCanvas("respCan", "respCan");
  respCan->Divide(2, 1);
  respCan->cd(1);
  respPos->Draw("A*");
  respPos->GetXaxis()->SetTitle("Chip temperature [C]");
  respPos->GetYaxis()->SetTitle("Response [ADC]");
  respCan->cd(2);
  respNeg->Draw("A*");
  respNeg->GetXaxis()->SetTitle("Chip temperature [C]");
  respNeg->GetYaxis()->SetTitle("Response [ADC]");
  respCan->Modified();
  respCan->Update();

  return;
}
