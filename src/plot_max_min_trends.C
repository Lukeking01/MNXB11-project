void plot_max_min_trends(const char* filename, const char* city = "City") {
    TFile *f = TFile::Open(filename);


    // Try both possible tree names
    TTree *temps = (TTree*)f->Get("temps");
    if (!temps) temps = (TTree*)f->Get("temps");
    if (!temps) {
        std::cerr << "No valid TTree found in " << filename << std::endl;
        return;
    }

    // Canvas setup
    auto c = new TCanvas("c", Form("%s Max/Min Temperature Trends", city), 900, 600);
    c->SetGrid();
    c->SetTitle(Form("%s: Maximum and Minimum Temperatures", city));
    
    // Profiles for averaging over year
    TProfile *pMax = new TProfile("pMax",
        Form("%s: Maximum and Minimum Temperatures;Year;Temperature [#circC]", city),
        150, 1850, 2025);
    TProfile *pMin = (TProfile*)pMax->Clone("pMin");

    // Fill profiles
    temps->Draw("max_temp:year >> pMax", "", "prof");
    temps->Draw("min_temp:year >> pMin", "", "prof same");

    // Style
    pMax->SetLineColor(kRed);
    pMax->SetMarkerColor(kRed);
    pMax->SetMarkerStyle(20);
    pMin->SetLineColor(kBlue);
    pMin->SetMarkerColor(kBlue);
    pMin->SetMarkerStyle(21);

    pMax->SetStats(0);
    pMin->SetStats(0);

    pMax->Draw("E1");
    pMin->Draw("E1 same");
    pMax->GetYaxis()->SetRangeUser(-30, 40);

    // Fit linear trends
    TF1 *fitMax = new TF1("fitMax", "pol1", 1850, 2025);
    TF1 *fitMin = new TF1("fitMin", "pol1", 1850, 2025);

    pMax->Fit(fitMax, "Q");
    pMin->Fit(fitMin, "Q");

    pMax->GetXaxis()->CenterTitle(true);
    pMax->GetYaxis()->CenterTitle(true);
    pMin->GetXaxis()->CenterTitle(true);
    pMin->GetYaxis()->CenterTitle(true);

    fitMax->SetLineColor(kRed+2);
    fitMin->SetLineColor(kBlue+2);
    fitMax->Draw("same");
    fitMin->Draw("same");

    // Legend
    auto legend = new TLegend(0, 0, 0.3, 0.2);
    legend->SetHeader(Form("%s: Max/Min Temperature Trends", city), "C");
    legend->AddEntry(pMax, "Max temperature", "lep");
    legend->AddEntry(fitMax, Form("Max trend: %.2f #pm %.2f C/century",
                                  100*fitMax->GetParameter(1), 100*fitMax->GetParError(1)), "l");
    legend->AddEntry(pMin, "Min temperature", "lep");
    legend->AddEntry(fitMin, Form("Min trend: %.2f #pm %.2f C/century",
                                  100*fitMin->GetParameter(1), 100*fitMin->GetParError(1)), "l");
    legend->Draw();

    // Save plot
    
    c->SaveAs(Form("plots/max_min_temps/%s_max_min_trends.pdf", city));

    std::cout << "  Saved " << city << " plot with max/min trends." << std::endl;
    std::cout << "   Max trend = " << 100*fitMax->GetParameter(1)
              << " °C/century, Min trend = " << 100*fitMin->GetParameter(1) << " °C/century" << std::endl;

    f->Close();
}
