#include <TFile.h>
#include <TTree.h>
#include <TH2D.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TStyle.h>
#include <iostream>

// --------------------
// 1️ Year vs Temperature
// --------------------
void plotYearVsTemperature(TTree* tree) {
    if (!tree->GetBranch("temperature")) return;

    TCanvas *c1 = new TCanvas("c1", "Year vs Temperature", 800, 600);
    tree->Draw("temperature:year", "temperature>0", "COLZ");
    c1->SetLogz();
    c1->SaveAs("year_vs_temperature.png");
    std::cout << "Saved year_vs_temperature.png" << std::endl;
}

// --------------------
// 2️ Year vs Max/Min Temperature
// --------------------
void plotYearVsMaxMin(TTree* tree) {
    if (!tree->GetBranch("max_temp") || !tree->GetBranch("min_temp")) return;

    TCanvas *c2 = new TCanvas("c2", "Year vs Max/Min Temperature", 800, 600);

    TH2D *hMax = new TH2D("hMax", "Year vs Max Temperature;Year;Max Temp", 100, 1800, 2100, 100, -50, 50);
    TH2D *hMin = new TH2D("hMin", "Year vs Min Temperature;Year;Min Temp", 100, 1800, 2100, 100, -50, 50);

    tree->Draw("max_temp:year>>hMax", "max_temp!=0");
    tree->Draw("min_temp:year>>hMin", "min_temp!=0");

    hMax->SetMarkerColor(kRed);
    hMax->SetMarkerStyle(20);
    hMin->SetMarkerColor(kBlue);
    hMin->SetMarkerStyle(21);

    hMax->Draw("P");
    hMin->Draw("P SAME");

    TLegend *leg = new TLegend(0.7,0.8,0.9,0.9);
    leg->AddEntry(hMax, "Max Temp", "p");
    leg->AddEntry(hMin, "Min Temp", "p");
    leg->Draw();

    c2->SaveAs("year_vs_maxmin.png");
    std::cout << "Saved year_vs_maxmin.png" << std::endl;
}

// --------------------
// Main function to call all
// --------------------
void plot_climate(const char* filename = "data.root") {
    gStyle->SetOptStat(1110); // Show stats box

    TFile *f = TFile::Open(filename);
    if (!f || f->IsZombie()) {
        std::cerr << "Cannot open file " << filename << std::endl;
        return;
    }

    TTree *tree = (TTree*)f->Get("temps");
    if (!tree) {
        std::cerr << "Tree 'temps' not found" << std::endl;
        return;
    }

    plotYearVsTemperature(tree);
    plotYearVsMaxMin(tree);

    f->Close();
    std::cout << "All plots done!" << std::endl;
}
