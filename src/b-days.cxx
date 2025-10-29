#include <iostream>
#include <fstream>
#include <sstream>    
#include <string>
#include <map>      
#include <vector> 
#include <tuple>
#include <algorithm>
#include <cctype>
#include <locale>


void filter_time(const char* inputFile = "datasets/B-days/Lund.csv",
                         const char* outputFile = "datasets/B-days/Lund_time.csv",
                         int startHour = 10,
                         int stopHour = 15){
    std::ifstream in(inputFile);
    std::ofstream out(outputFile);

    std::string line;
    int total=0, kept=0;

    while (std::getline(in, line)) //true if can read line
    {
        total++;
        std::stringstream ss(line); //want to read pieces of the line
        std::string col; //store column

        for (int i = 0; i < 3; ++i) std::getline(ss, col, ';');
        if (!std::getline(ss, col, ';')) continue;
        int hour = std::stoi(col);
        if (hour >= startHour && hour <= stopHour) {
            out << line << "\n";
            kept++;
        }
    }
    std::cout<< " lines, kept " << kept << " lines.\n";
    }
    
void yearly_avg(const char* inputFile = "datasets/B-days/Lund_time.csv", 
                const char* outputFile = "datasets/B-days/Lund_avg.csv"){
    std::ifstream in(inputFile);

    std::map<std::tuple<int,int,int>, std::pair<double,int>> data;
    std::string line;
    int total =0;


    while (std::getline(in, line))
    {
        total++;
        std::stringstream ss(line);
        int year, month, day, hour;
        double temp;
        char sep;

        ss >> year >> sep >> month >> sep >> day >> sep >> hour >> sep >> temp;

        auto key= std::make_tuple(year, month, day);
        data[key].first += temp;
        data[key].second++;
    }

    std::ofstream out(outputFile);
    
    int written =0;

    for (const auto& [key, val] : data) {
        auto [year, month, day] = key;
        double avg = val.first / val.second;
        out << year << ";" << month << ";" << day << ";" << avg << "\n";
        written++;
    }
    std::cout << "Averages written to " << outputFile
              << " (" << data.size() << " entries)\n";
    std::cout << "Lines read: " << total << ", lines written: " << written 
              << " (" << data.size() << " unique days)\n";

}

void points(const char* inputFile="datasets/B-days/Lund_avg.csv",
            const char* outputFile="datasets/B-days/Lund_points.csv"){
    std::string line;
    std::ifstream in(inputFile);

    // separate data for each day -> vectors of year,temp
    std::map<std::pair<int,int>, std::vector<std::pair<int,double>>> data;
    int year, month, day;
    double avg;

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;

         while (std::getline(ss, token, ';')) {
            tokens.push_back(token);
        }
        
        
        year = std::stoi(tokens[0]);
        month = std::stoi(tokens[1]);
        day = std::stoi(tokens[2]);
        avg = std::stod(tokens[3]);

    if ((month==11 && day==6) || (month==3 && day==11) || (month==4 && day==12)) {
        data[{month, day}].push_back({year, avg});
    }
}
    
    std::ofstream out(outputFile);
    

    for (const auto& [key, vec] : data) {
        auto [month, day] = key;
        for (const auto& [year, avg] : vec) {
            out << year << ";" << month << ";" << day << ";" << avg << "\n";
        }
    }

    std::cout << "Filtered data written to " << outputFile 
              << " (" << data.size() << " day groups)\n";
}



int main() {
    filter_time();
    yearly_avg();
    points();
}



