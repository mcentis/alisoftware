void makeUpAxis(TGraph* in, float size)
{
  in->GetXaxis()->SetTitleSize(size);
  in->GetXaxis()->SetLabelSize(size);

  in->GetYaxis()->SetTitleSize(size);
  in->GetYaxis()->SetLabelSize(size);

  return;
}

void makeUpAxis(TH1* in, float size)
{
  in->GetXaxis()->SetTitleSize(size);
  in->GetXaxis()->SetLabelSize(size);

  in->GetYaxis()->SetTitleSize(size);
  in->GetYaxis()->SetLabelSize(size);

  return;
}
