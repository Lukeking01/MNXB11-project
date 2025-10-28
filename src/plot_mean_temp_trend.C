
#include <TFile.h>
#include <TTree.h>
#include <TH2D.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TStyle.h>
#include <iostream>

void plot_mean_temp_trend(const char* filename, const char* city = "City") {
    TFile *f = TFile::Open(filename);
    TTree *temp = (TTree*)f->Get("temps");

    auto c = new TCanvas("c", city, 800, 600);
    auto p = new TProfile("p", Form("%s Mean Temperature;Year;Mean Temp [#circC]", city),
                          150, 1850, 2025);

    temp->Draw("0.5*(max_temp+min_temp):year >> p", "", "prof");
    p->SetLineColor(kBlue);
    p->SetMarkerStyle(20);
    p->Draw("E1");

    TF1 *fit = new TF1("fit", "pol1", 1850, 2025);
    fit->SetLineColor(kRed);
    p->Fit(fit, "Q");
    fit->Draw("SAME");

    auto legend = new TLegend(0, 0, 0.3, 0.2);
    legend->AddEntry(p, "Mean temperature", "lep");
    legend->AddEntry(fit, "Linear fit", "l");
    legend->Draw();

    c->SaveAs(Form("plots/mean_temps/%s_mean_trend.pdf", city));
}
