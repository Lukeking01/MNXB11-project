#include <iostream>
#include <fstream>
#include <sstream>    
#include <string>
#include <map>      
#include <vector> 
#include <tuple>
#include <TCanvas.h>
#include <TGraph.h>
#include <TMultiGraph.h>
#include <TAxis.h>

void plot_bdays(const char* filename, const char* city="City"){
    // --- Open the CSV file produced by points() ---
    std::ifstream in(filename);
    if (!in.is_open()) {
        std::cerr << "Error: cannot open " << filename << std::endl;
        return;
    }

    std::string line;
    std::getline(in, line); // skip header

    // Map: (month, day) -> vector of (year, avg temp)
    std::map<std::pair<int,int>, std::vector<std::pair<int,double>>> data;

    while (std::getline(in, line)) {
        std::stringstream ss(line);
        int year, month, day;
        double avg;
        char sep;

        ss >> year >> sep >> month >> sep >> day >> sep >> avg;
        if (ss.fail()) continue;

        data[{month, day}].push_back({year, avg});
    }
    in.close();

    
    TCanvas* c = new TCanvas("c", Form("Average Temperature - %s", city), 900, 600);
    TMultiGraph* mg = new TMultiGraph();
    TLegend* legend;
    if (std::string(city) == "Lund")
    {
        legend = new TLegend(0.2, 0.8, 0.4, 0.95); 

    }
    else 
    {
        legend = new TLegend(0.70, 0.18, 0.90, 0.33);
    }
    legend->SetTextSize(0.03);    
        legend->SetFillStyle(0);  
        legend->SetBorderSize(1); 
         


    int color = 2; // start color for first birthday
    for (auto& [key, vec] : data) {
        std::vector<double> x, y;
        for (auto& [year, avg] : vec) {
            x.push_back(year);
            y.push_back(avg);
        }

        TGraph* gr = new TGraph(x.size(), x.data(), y.data());
        gr->SetLineColor(color);
        gr->SetMarkerColor(color);
        gr->SetMarkerStyle(20);
        gr->SetTitle(Form("%02d-%02d", key.first, key.second)); // month-day
        mg->Add(gr);
        legend->AddEntry(gr, Form("%02d-%02d", key.first, key.second), "lp");

        color++; // next birthday gets a different color
    }

    mg->SetTitle(Form("Average Temperature in %s;Year;Temperature (#circC)", city));
    mg->Draw("APL");

    legend->Draw();
    c->Update();
    c->Draw();

    c->SaveAs(Form("plots/bdays/%sbdays.pdf", city));
    }

