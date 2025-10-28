#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " input.csv [output.root]"
              << std::endl;
    return 1;
  }
  std::string city = argv[1];
  std::ifstream input("datasets/clean/" + city);
  if (!input.is_open()) {
    std::cerr << "Could not open " << city << ".csv\n";
    return 1;
  }

  std::vector<std::vector<std::string>> data;
  std::string line;

  while (std::getline(input, line)) {
    std::vector<std::string> row;
    std::stringstream ss(line);
    std::string cell;

    while (std::getline(ss, cell, ';')) {
      row.push_back(cell);
    }
    data.push_back(row);
  }

  input.close();

  std::ofstream output("datasets/Climate/" + city);
  if (!output.is_open()) {
    std::cerr << "Could not open " << city << ".csv\n";
    return 1;
  }

  int year{std::stoi(data[0][0])};
  double max{0};
  double min{0};
  int y;
  double t;
  for (auto& r : data) {
    y = std::stoi(r[0]);
    if (y != year) {
      output << year << "; " << max << "; " << min << std::endl;
      year = y;
      max = 0;
      min = 0;
    }
    t = std::stod(r[4]);
    if (t < min) {
      min = t;
    }
    if (t > max) {
      max = t;
    }
  }
  output << year << "; " << max << "; " << min << std::endl;

  output.close();

  std::cout << "Data written to " << city <<"\n";
  return 0;
}