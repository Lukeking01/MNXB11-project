#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <filesystem>

namespace fs = std::filesystem;

struct YearData {
    double max_sum = 0;
    double min_sum = 0;
    int count = 0;
};

int main() {
    std::string folder = "datasets/Climate"; 
    std::map<int, YearData> averages;

    // Loop over all CSV files in the folder
    for (const auto& entry : fs::directory_iterator(folder)) {
        if (entry.path().extension() != ".csv") continue;

        std::ifstream fin(entry.path());
        if (!fin.is_open()) {
            std::cerr << "Cannot open " << entry.path() << std::endl;
            continue;
        }

        std::string line;
        std::getline(fin, line); // skip header

        while (std::getline(fin, line)) {
            std::stringstream ss(line);
            std::string year_str, max_str, min_str;

            if (!std::getline(ss, year_str, ';')) continue;
            if (!std::getline(ss, max_str, ';')) continue;
            if (!std::getline(ss, min_str, ';')) continue;

            try {
                int year = std::stoi(year_str);
                double max_temp = std::stod(max_str);
                double min_temp = std::stod(min_str);

                averages[year].max_sum += max_temp;
                averages[year].min_sum += min_temp;
                averages[year].count += 1;
            } catch (...) {
                // skip malformed lines
                continue;
            }
        }

        fin.close();
    }

    // Write averaged CSV
    std::ofstream fout("datasets/Climate/Sweden.csv");
    

    for (const auto& [year, data] : averages) {
        if (data.count == 0) continue;
        double avg_max = data.max_sum / data.count;
        double avg_min = data.min_sum / data.count;
        

        fout << year << ";" << avg_max << ";" << avg_min << "\n";
    }

    fout.close();
    std::cout << "Averaged CSV saved to Sweden.csv\n";
}
