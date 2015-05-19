void calVsT()
{
  const char* baseDir = "./data/M200Punirrad/filesCalVsT/"; // it is important to use the final /
  // runs and temperatures for delay 78
  const int nPoints = 6;
  double temp[nPoints] = {13, 14, 19, 22, 23.6, 24};
  char* runs[nPoints] = {"run000017.root", "run000013.root", "run000010.root", "run000007.root", "run000004.root", "run000002.root"};

  // runs and temperatures for delay 82
  // const int nPoints = 4;
  // double temp[nPoints] = {13, 14, 19, 22};
  // char* runs[nPoints] = {"run000016.root", "run000012.root", "run000009.root", "run000006.root"};

  TGraphErrors* slopePos = new TGraphErrors();
  slopePos->SetName("slopePos");
  slopePos->SetTitle("slope pos cal");

  TGraphErrors* offsetPos = new TGraphErrors();
  offsetPos->SetName("offsetPos");
  offsetPos->SetTitle("offset pos cal");

  TGraphErrors* slopeNeg = new TGraphErrors();
  slopeNeg->SetName("slopeNeg");
  slopeNeg->SetTitle("slope neg cal");

  TGraphErrors* offsetNeg = new TGraphErrors();
  offsetNeg->SetName("offsetNeg");
  offsetNeg->SetTitle("offset neg cal");

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
      sprintf(fileName, "%s/%s", baseDir, runs[i]);
      inFile = TFile::Open(fileName);

      // std::cout << fileName << std::endl;
      if(inFile == 0) continue;

      std::cout << "--------------- chip temperature " << temp[i] << std::endl;

      dir = (TDirectory*) inFile->Get("Graphs_posCal_Parameters");
      hist = (TH1*) dir->Get("distrPar_1_posCal_goodCh");
      slopePos->SetPoint(i, temp[i], hist->GetMean());
      slopePos->SetPointError(i, 0, hist->GetRMS() / sqrt(hist->GetEntries()));
      slope = hist->GetMean();
      eSlope = hist->GetRMS() / sqrt(hist->GetEntries());

      hist = (TH1*) dir->Get("distrPar_0_posCal_goodCh");
      offsetPos->SetPoint(i, temp[i], hist->GetMean());
      offsetPos->SetPointError(i, 0, hist->GetRMS() / sqrt(hist->GetEntries()));
      offset = hist->GetMean();
      eOff = hist->GetRMS() / sqrt(hist->GetEntries());

      respPos->SetPoint(i, temp[i], charge * slope + offset);
      err = sqrt(charge * charge * eSlope * eSlope + eOff * eOff);
      respPos->SetPointError(i, 0, err);

      std::cout << "pos cal slope " << slope << " offset " << offset << std::endl;

      dir = (TDirectory*) inFile->Get("Graphs_negCal_Parameters");
      hist = (TH1*) dir->Get("distrPar_1_negCal_goodCh");
      slopeNeg->SetPoint(i, temp[i], hist->GetMean());
      slopeNeg->SetPointError(i, 0, hist->GetRMS() / sqrt(hist->GetEntries()));
      slope = hist->GetMean();
      eSlope = hist->GetRMS() / sqrt(hist->GetEntries());

      hist = (TH1*) dir->Get("distrPar_0_negCal_goodCh");
      offsetNeg->SetPoint(i, temp[i], hist->GetMean());
      offsetNeg->SetPointError(i, 0, hist->GetRMS() / sqrt(hist->GetEntries()));
      offset = hist->GetMean();
      eOff = hist->GetRMS() / sqrt(hist->GetEntries());

      respNeg->SetPoint(i, temp[i], charge * slope + offset);
      err = sqrt(charge * charge * eSlope * eSlope + eOff * eOff);
      respNeg->SetPointError(i, 0, err);

      std::cout << "neg cal slope " << slope << " offset " << offset << std::endl;

      inFile->Close();
    }

  std::cout << "readen files" << std::endl;

  TF1* fitPosSlope = new TF1("fitPosSlope", "pol2", 0, 30);
  TF1* fitNegSlope = new TF1("fitNegSlope", "pol2", 0, 30);
  fitNegSlope->SetParameters(0.004, 7e-5, -4e-6); // start values in case of fit with errors in T

  TCanvas* posCan = new TCanvas("posCan", "posCan");
  posCan->Divide(2, 1);
  posCan->cd(1);
  slopePos->Draw("AP");
  slopePos->Fit(fitPosSlope);
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
  slopeNeg->Fit(fitNegSlope);
  slopeNeg->GetXaxis()->SetTitle("Chip temperature [C]");
  slopeNeg->GetYaxis()->SetTitle("Slope fit [ADC / e]");
  negCan->cd(2);
  offsetNeg->Draw("AP");
  offsetNeg->GetXaxis()->SetTitle("Chip temperature [C]");
  offsetNeg->GetYaxis()->SetTitle("Offset fit [ADC]");
  negCan->Modified();
  negCan->Update();

  TCanvas* posFuncCan = new TCanvas("posFuncCan", "posFuncCan");
  fitPosSlope->Draw();
  //slopePos->Draw("Psame");
  posFuncCan->Modified();
  posFuncCan->Update();

  TCanvas* negFuncCan = new TCanvas("negFuncCan", "negFuncCan");
  fitNegSlope->Draw();
  //slopeNeg->Draw("Psame");
  negFuncCan->Modified();
  negFuncCan->Update();

  // TCanvas* respCan = new TCanvas("respCan", "respCan");
  // respCan->Divide(2, 1);
  // respCan->cd(1);
  // respPos->Draw("A*");
  // respPos->GetXaxis()->SetTitle("Chip temperature [C]");
  // respPos->GetYaxis()->SetTitle("Response [ADC]");
  // respCan->cd(2);
  // respNeg->Draw("A*");
  // respNeg->GetXaxis()->SetTitle("Chip temperature [C]");
  // respNeg->GetYaxis()->SetTitle("Response [ADC]");
  // respCan->Modified();
  // respCan->Update();

  TF1* errSlope = new TF1("errSlope", "sqrt(pow([0], 2) + pow(x * [1], 2) + pow(x * x * [2], 2) + pow(([3] + 0.5 * [4] * x) * [5], 2))", 0, 30);
  errSlope->SetParameter(0, fitNegSlope->GetParError(0));
  errSlope->SetParameter(1, fitNegSlope->GetParError(1));
  errSlope->SetParameter(2, fitNegSlope->GetParError(2));
  errSlope->SetParameter(3, fitNegSlope->GetParameter(1));
  errSlope->SetParameter(4, fitNegSlope->GetParameter(2));
  errSlope->SetParameter(5, 0);// error on T

  TCanvas* errNegSlopeCan = new TCanvas("errNegSlopeCan", "errNegSlopeCan");
  errSlope->Draw();

  double slope1 = fitNegSlope->Eval(10.43);
  double err1 = errSlope->Eval(10.43);
  double slope2 = fitNegSlope->Eval(23.8);
  double err2 = errSlope->Eval(23.8);
  double ratio = slope1 / slope2;
  double errRatio = sqrt(pow(err1 / slope1, 2) + pow(err2 / slope2, 2)) * ratio;

  std::cout << "slope at 10.43 C " << slope1 << " +- " << err1 << std::endl;
  std::cout << "slope at 23.8 C  " << slope2 << " +- " << err2 << std::endl;
  std::cout << "slope ratio      " << ratio << " +- " << errRatio << std::endl;

  return;
}
