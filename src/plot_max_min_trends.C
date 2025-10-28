#include <TFile.h>
#include <TTree.h>
#include <TProfile.h>
#include <TGraphErrors.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TF1.h>
#include <iostream>

void plot_max_min_trends(const char* filename, const char* city = "City") {
    TFile *f = TFile::Open(filename);

    TTree *temps = (TTree*)f->Get("temps");
    if (!temps) {
        std::cerr << "No valid TTree found in " << filename << std::endl;
        return;
    }

    auto c = new TCanvas("c", Form("%s Max/Min Temperature Trends", city), 900, 600);
    c->SetGrid();
    c->SetTitle(Form("%s: Maximum and Minimum Temperatures", city));

    // Use yearly profiles
    TProfile *pMax = new TProfile("pMax",
        Form("%s: Maximum and Minimum Temperatures;Year;Temperature [#circC]", city),
        150, 1850, 2025);
    TProfile *pMin = (TProfile*)pMax->Clone("pMin");

    temps->Draw("max_temp:year >> pMax", "", "prof");
    temps->Draw("min_temp:year >> pMin", "", "prof same");

    // Convert TProfile to TGraphErrors for fitting
    auto graphMax = new TGraphErrors();
    auto graphMin = new TGraphErrors();
    int pointIndex = 0;
    for (int i = 1; i <= pMax->GetNbinsX(); i++) {
        if (pMax->GetBinEntries(i) > 0) {
            double x = pMax->GetBinCenter(i);
            double y = pMax->GetBinContent(i);
            double ey = pMax->GetBinError(i);
            graphMax->SetPoint(pointIndex, x, y);
            graphMax->SetPointError(pointIndex, 0, ey);
            pointIndex++;
        }
    }

    pointIndex = 0;
    for (int i = 1; i <= pMin->GetNbinsX(); i++) {
        if (pMin->GetBinEntries(i) > 0) {
            double x = pMin->GetBinCenter(i);
            double y = pMin->GetBinContent(i);
            double ey = pMin->GetBinError(i);
            graphMin->SetPoint(pointIndex, x, y);
            graphMin->SetPointError(pointIndex, 0, ey);
            pointIndex++;
        }
    }

    // Style
    graphMax->SetLineColor(kRed);
    graphMax->SetMarkerColor(kRed);
    graphMax->SetMarkerStyle(20);
    graphMin->SetLineColor(kBlue);
    graphMin->SetMarkerColor(kBlue);
    graphMin->SetMarkerStyle(21);

    graphMax->SetTitle(Form("%s: Maximum and Minimum Temperatures;Year;Temperature [#circC]", city));
    graphMax->GetXaxis()->CenterTitle(true);
    graphMax->GetYaxis()->CenterTitle(true);

    graphMax->Draw("AP");
    graphMin->Draw("P same");
    graphMax->GetYaxis()->SetRangeUser(-30, 50);

    // Fit linear trends
    TF1 *fitMax = new TF1("fitMax", "pol1", 1900, 2024);
    TF1 *fitMin = new TF1("fitMin", "pol1", 1900, 2024);

    graphMax->Fit(fitMax, "Q0");
    graphMin->Fit(fitMin, "Q0");

    fitMax->SetLineColor(kRed+2);
    fitMin->SetLineColor(kBlue+2);
    fitMax->Draw("same");
    fitMin->Draw("same");

    // Legend
    auto legend = new TLegend(0.7, 0.8, 1, 1);
    legend->SetHeader(Form("%s: Max/Min Temperature Trends", city), "C");
    legend->AddEntry(graphMax, "Max temperature", "lep");
    legend->AddEntry(fitMax, Form("Max trend: %.2f #pm %.2f #circC/century",
                                  100*fitMax->GetParameter(1), 100*fitMax->GetParError(1)), "l");
    legend->AddEntry(graphMin, "Min temperature", "lep");
    legend->AddEntry(fitMin, Form("Min trend: %.2f #pm %.2f #circC/century",
                                  100*fitMin->GetParameter(1), 100*fitMin->GetParError(1)), "l");
                                  legend->Draw();

    c->SaveAs(Form("plots/max_min_temps/%s_max_min_trends.pdf", city));

    std::cout << "  Saved " << city << " plot with max/min trends." << std::endl;
    std::cout << "   Max trend = " << 100*fitMax->GetParameter(1)
              << " °C/century, Min trend = " << 100*fitMin->GetParameter(1) << " °C/century" << std::endl;

    f->Close();
}
