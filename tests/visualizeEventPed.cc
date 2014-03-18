void visualizeEventPed(int evtNum)
{
  const int nChannels = 256;

  Float_t commMode;
  UInt_t rawPH[nChannels] = {0};
  rawEvtTree->SetBranchAddress("adcPH", rawPH);
  rawEvtTree->SetBranchAddress("commMode", &commMode);

  rawEvtTree->GetEntry(evtNum);

  double pedSubPH[nChannels] = {0};
  double pedSubPHgood[nChannels] = {0};
  double commMSubPH[nChannels] = {0}; // signal

  double* ped = PedGraph->GetY(); // not self consistent....
  double* goodCH = NoiseGraph->GetX();
  int nGoodCH = NoiseGraph->GetN();

  bool isGood[nChannels] = {0};
  for(int i = 0; i < nGoodCH; ++i) isGood[(int) goodCH[i]] = true;
  // for(int i = 0; i < nChannels; ++i) std::cout << i << "   " << isGood[i] << std::endl;

  for(int i = 0; i < nChannels; ++i)
    {
      pedSubPH[i] = rawPH[i] - ped[i];
      if(isGood[i] == false) continue; // exlude bad channels for the next plot
      pedSubPHgood[i] = rawPH[i] - ped[i];
      commMSubPH[i] = pedSubPH[i] - commMode;
    }

  TCanvas* evtCan = new TCanvas("evtCan", "Pulse height and common mode");
  evtCan->Divide(2,2);

  TH1I* rawhist = new TH1I("rawhist", TString::Format("Raw pulse height event %i;Channel;PH [ADC]", evtNum), 256, -0.5, 255.5);
  TH1D* pedsubhist = new TH1D("pedsubhist", TString::Format("Pedestal subtracted pulse height event %i;Channel;PH [ADC]", evtNum), 256, -0.5, 255.5);
  TH1D* pedsubgoodhist = new TH1D("pedsubgoodhist", TString::Format("Pedestal subtracted pulse height good channels event %i;Channel;PH [ADC]", evtNum), 256, -0.5, 255.5);
  TH1D* commsubhist = new TH1D("commsubhist", TString::Format("Common mode subtracted pulse height event %i;Channel;PH [ADC]", evtNum), 256, -0.5, 255.5);

  TF1* cm = new TF1("cm", "[0]", -0.5, 255.5);
  cm->SetParameter(0, commMode);
  cm->SetLineColor(kRed);

  TH1* histos[4] = {rawhist, pedsubhist, pedsubgoodhist, commsubhist};
  for(int i = 0; i < 4; i++)
    {
      histos[i]->SetLineColor(kBlue);
      histos[i]->SetLineWidth(1);
    }

  for(int i = 0; i < nChannels; ++i)
    {
      rawhist->SetBinContent(i + 1, rawPH[i]);
      pedsubhist->SetBinContent(i + 1, pedSubPH[i]);
      pedsubgoodhist->SetBinContent(i + 1, pedSubPHgood[i]);
      commsubhist->SetBinContent(i + 1, commMSubPH[i]);
    }

  evtCan->cd(1);
  rawhist->Draw();

  evtCan->cd(2);
  pedsubhist->Draw();

  evtCan->cd(3);
  pedsubgoodhist->Draw();
  cm->Draw("SAME");

  evtCan->cd(4);
  commsubhist->Draw();

  return;
}
