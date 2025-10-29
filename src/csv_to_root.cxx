#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Full usage: 
// g++ csv_to_root.cxx $(root-config --cflags --libs) -o csv_to_root 
// ./csv_to_root input.csv [output.root] 
// TFile *f = TFile::Open("file.root") 
// TTree *temps = (TTree*)f->Get("temps") 
// temps->Draw("temperature:year")

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " input.csv [output.root]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile;

    if (argc >= 3)
        outputFile = argv[2];
    else
        outputFile = inputFile.substr(0, inputFile.find_last_of(".")) + ".root";

    std::ifstream infile(inputFile);
    if (!infile.is_open()) {
        std::cerr << "❌ Error: could not open file " << inputFile << std::endl;
        return 1;
    }

    TFile *outfile = new TFile(outputFile.c_str(), "RECREATE");
    TTree *tree = new TTree("temps", "Climate data from CSV");

    // Variables for full CSV
    int year=0, month=0, day=0, hour=0;
    double temperature=0, longitude=0, latitude=0;

    // Variables for minimal CSV
    double max_temp=0, min_temp=0, mean_temp=0;

    // Create branches for both possibilities
    tree->Branch("year", &year, "year/I");
    tree->Branch("month", &month, "month/I");
    tree->Branch("day", &day, "day/I");
    tree->Branch("hour", &hour, "hour/I");
    tree->Branch("temperature", &temperature, "temperature/D");
    tree->Branch("longitude", &longitude, "longitude/D");
    tree->Branch("latitude", &latitude, "latitude/D");

    tree->Branch("max_temp", &max_temp, "max_temp/D");
    tree->Branch("min_temp", &min_temp, "min_temp/D");
    tree->Branch("mean_temp", &mean_temp, "mean_temp/D");

    std::string line;
    long nLines = 0;

    while (std::getline(infile, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;

        // Split line into tokens
        while (std::getline(ss, token, ';')) {
            tokens.push_back(token);
        }

        if (tokens.size() == 7) {
            // Full CSV
            year = std::stoi(tokens[0]);
            month = std::stoi(tokens[1]);
            day = std::stoi(tokens[2]);
            hour = std::stoi(tokens[3]);
            temperature = std::stod(tokens[4]);
            longitude = std::stod(tokens[5]);
            latitude = std::stod(tokens[6]);

            max_temp = min_temp = 0; // unused
        }
        else if (tokens.size() == 4) {
            // Minimal CSV
            year = std::stoi(tokens[0]);
            max_temp = std::stod(tokens[1]);
            min_temp = std::stod(tokens[2]);
            mean_temp = std::stod(tokens[3]);

            month = day = hour = 0;
            temperature = longitude = latitude = 0;
        }
        else {
            std::cerr << "⚠️ Skipping line with unexpected column count: " << line << std::endl;
            continue;
        }

        tree->Fill();
        ++nLines;
    }

    outfile->Write();
    outfile->Close();

    std::cout << "Wrote " << nLines << " rows to " << outputFile << std::endl;
    return 0;
}
