#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// Usage: ./csv_to_root input.csv [output.root]
// CSV format: year,month,day,hour,temperature,longitude,latitude

// Full usage:
// g++ ../../src/csv_to_root.cxx $(root-config --cflags --libs) -o csv_to_root
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

    // Default output name = same as input but with .root
    if (argc >= 3)
        outputFile = argv[2];
    else
        outputFile = inputFile.substr(0, inputFile.find_last_of(".")) + ".root";

    std::ifstream infile(inputFile);
    if (!infile.is_open()) {
        std::cerr << "❌ Error: could not open file " << inputFile << std::endl;
        return 1;
    }

    // Create ROOT file and tree
    TFile *outfile = new TFile(outputFile.c_str(), "RECREATE");
    TTree *tree = new TTree("temps", "Climate data from CSV (no header)");

    // Variables for reading
    int year, month, day, hour;
    double temperature, longitude, latitude;

    // Link to branches
    tree->Branch("year", &year, "year/I");
    tree->Branch("month", &month, "month/I");
    tree->Branch("day", &day, "day/I");
    tree->Branch("hour", &hour, "hour/I");
    tree->Branch("temperature", &temperature, "temperature/D");
    tree->Branch("longitude", &longitude, "longitude/D");
    tree->Branch("latitude", &latitude, "latitude/D");

    std::string line;
    long nLines = 0;

    while (std::getline(infile, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string token;

        // Parse 7 comma-separated values
        if (!std::getline(ss, token, ',')) continue;  year = std::stoi(token);
        if (!std::getline(ss, token, ',')) continue;  month = std::stoi(token);
        if (!std::getline(ss, token, ',')) continue;  day = std::stoi(token);
        if (!std::getline(ss, token, ',')) continue;  hour = std::stoi(token);
        if (!std::getline(ss, token, ',')) continue;  temperature = std::stod(token);
        if (!std::getline(ss, token, ',')) continue;  longitude = std::stod(token);
        if (!std::getline(ss, token, ',')) continue;  latitude = std::stod(token);

        tree->Fill();
        ++nLines;
    }

    outfile->Write();
    outfile->Close();

    std::cout << "✅ Wrote " << nLines << " rows to " << outputFile << std::endl;
    return 0;
}
