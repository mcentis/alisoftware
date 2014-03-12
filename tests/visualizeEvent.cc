void visualizeEvent(int evtNum)
{
  const int nChannels = 256;

  Float_t ped[nChannels] = {0};
  Float_t noise[nChannels] = {0};
  chPropTree->SetBranchAddress("pedestals", ped);
  chPropTree->SetBranchAddress("noise", noise);

  chPropTree->GetEntry(0);

  Float_t commMode;
  cookedEvtTree->SetBranchAddress("commMode", &commMode);

  UInt_t rawPH[nChannels] = {0};
  rawEvtTree->SetBranchAddress("adcPH", rawPH);

  rawEvtTree->GetEntry(evtNum);
  cookedEvtTree->GetEntry(evtNum);

  double pedSubPH[nChannels] = {0};
  double pedSubPHgood[nChannels] = {0};
  double commMSubPH[nChannels] = {0}; // signal
  double snr[nChannels] = {0};

  for(int i = 0; i < nChannels; ++i)
    {
      pedSubPH[i] = rawPH[i] - ped[i];
      if(noise[i] == -1) continue; // exlude bad channels for the next plot
      pedSubPHgood[i] = rawPH[i] - ped[i];
      commMSubPH[i] = pedSubPH[i] - commMode;
    }

  for(int i = 0; i < nChannels; ++i)
    if(noise[i] != -1) snr[i] = fabs(commMSubPH[i]) / fabs(noise[i]);

  TCanvas* evtCan = new TCanvas("evtCan", "Pulse height and common mode");
  evtCan->Divide(3,2);

  TH1I* rawhist = new TH1I("rawhist", TString::Format("Raw pulse height event %i;Channel;PH [ADC]", evtNum), 256, -0.5, 255.5);
  TH1D* pedsubhist = new TH1D("pedsubhist", TString::Format("Pedestal subtracted pulse height event %i;Channel;PH [ADC]", evtNum), 256, -0.5, 255.5);
  TH1D* pedsubgoodhist = new TH1D("pedsubgoodhist", TString::Format("Pedestal subtracted pulse height good channels event %i;Channel;PH [ADC]", evtNum), 256, -0.5, 255.5);
  TH1D* commsubhist = new TH1D("commsubhist", TString::Format("Common mode subtracted pulse height event %i;Channel;PH [ADC]", evtNum), 256, -0.5, 255.5);
  TH1D* snrhist = new TH1D("snrhist", TString::Format("Signal to noise ratio event %i;Channel;PH [ADC]", evtNum), 256, -0.5, 255.5);

  TF1* cm = new TF1("cm", "[0]", -0.5, 255.5);
  cm->SetParameter(0, commMode);
  cm->SetLineColor(kRed);

  TH1* histos[5] = {rawhist, pedsubhist, pedsubgoodhist, commsubhist, snrhist};
  for(int i = 0; i < 5; i++)
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
      snrhist->SetBinContent(i + 1, snr[i]);
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

  evtCan->cd(5);
  snrhist->Draw();

  return;
}
