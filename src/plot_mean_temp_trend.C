#include <TFile.h>
#include <TTree.h>
#include <TProfile.h>
#include <TGraphErrors.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TF1.h>
#include <iostream>

void plot_mean_temp_trend(const char* filename, const char* city = "City") {
    TFile *f = TFile::Open(filename);
    TTree *temp = (TTree*)f->Get("temps");

    auto c = new TCanvas("c", city, 800, 600);

    // Create TProfile for yearly averages
    TProfile *p = new TProfile("p",
        Form("%s Mean Temperature;Year;Mean Temp [#circC]", city),
        150, 1850, 2025);

    temp->Draw("0.5*(max_temp+min_temp):year >> p", "", "prof");

    // Convert TProfile to TGraphErrors
    TGraphErrors *g = new TGraphErrors();
    int pointIndex = 0;
    for (int i = 1; i <= p->GetNbinsX(); i++) {
        if (p->GetBinEntries(i) > 0) {
            double x = p->GetBinCenter(i);
            double y = p->GetBinContent(i);
            double ey = p->GetBinError(i);
            g->SetPoint(pointIndex, x, y);
            g->SetPointError(pointIndex, 0, ey);
            pointIndex++;
        }
    }

    // Style
    g->SetLineColor(kBlue);
    g->SetMarkerStyle(20);
    g->SetTitle(Form("%s Mean Temperature;Year;Mean Temp [#circC]", city));
    g->GetXaxis()->CenterTitle(true);
    g->GetYaxis()->CenterTitle(true);
    g->GetYaxis()->SetRangeUser(-10, 15);

    g->Draw("AP");

    // Fit linear trend
    TF1 *fit = new TF1("fit", "pol1", 1850, 2025);
    fit->SetLineColor(kRed);
    g->Fit(fit, "Q");
    fit->Draw("SAME");

    // Legend
    auto legend = new TLegend(0.7, 0.8, 1, 1);
    legend->SetHeader(Form("%s Mean Temperature", city), "C");
    legend->AddEntry(g, "Mean temperature", "lep");
    //legend->AddEntry(fit, "Linear fit", "l");
    legend->AddEntry(fit, Form("Linear fit: %.2f #pm %.2f #circC/century",
                    100*fit->GetParameter(1), 100*fit->GetParError(1)), "l");
    legend->Draw();

    c->SaveAs(Form("plots/mean_temps/%s_mean_trend.pdf", city));
    std::cout << "Saved " << city << " mean temperature plot." << std::endl;

    f->Close();
}
