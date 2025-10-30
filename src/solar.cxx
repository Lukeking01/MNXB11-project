#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "TFile.h"
#include "TTree.h"

#ifdef year
#undef year
#endif

namespace fs = std::filesystem;

// ------------------ Solar math (UTC, on-the-hour) ------------------
constexpr double PI = 3.14159265358979323846;
constexpr double DEG2RAD = PI / 180.0;
constexpr double I_sc = 1367.0;  // W/m^2 (solar constant)

inline bool isLeap(int year) {
  return (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
}

inline int dayOfYear(int y, int m, int d) {
  static const int cum[12] = {0,   31,  59,  90,  120, 151,
                              181, 212, 243, 273, 304, 334};
  int maxd = (m == 2 ? (isLeap(y) ? 29 : 28)
                     : (m == 4 || m == 6 || m == 9 || m == 11 ? 30 : 31));
  if (m < 1 || m > 12 || d < 1 || d > maxd) return -1;
  int J = cum[m - 1] + d;
  if (m > 2 && isLeap(y)) ++J;
  return J;
}

inline double equationOfTime_min(int J) {
  double B = 2.0 * PI * (J - 81) / 364.0;
  return 9.87 * std::sin(2.0 * B) - 7.53 * std::cos(B) - 1.5 * std::sin(B);
}
inline double solarDeclination_rad(int J) {
  return 23.45 * DEG2RAD * std::sin(2.0 * PI * (284.0 + J) / 365.0);
}
inline double eccentricityCorr(int J) {
  return 1.0 + 0.033 * std::cos(2.0 * PI * J / 365.0);
}
inline double cosZenith(double lat_rad, double decl_rad,
                        double hour_angle_rad) {
  return std::sin(lat_rad) * std::sin(decl_rad) +
         std::cos(lat_rad) * std::cos(decl_rad) * std::cos(hour_angle_rad);
}

// Instantaneous TOA horizontal irradiance [W/m^2]
// Inputs: UTC hour (integer hour), lon/lat in degrees (east+/north+)
inline double toaHorizontalIrradiance_Wm2(int year, int month, int day,
                                          int hourUTC, double lon_deg,
                                          double lat_deg) {
  int J = dayOfYear(year, month, day);
  if (J < 1) return 0.0;

  const double EoT = equationOfTime_min(J);
  const double lst =
      hourUTC + (lon_deg * 4.0 + EoT) / 60.0;        // local solar time [h]
  const double H = (15.0 * (lst - 12.0)) * DEG2RAD;  // hour angle [rad]

  const double delta = solarDeclination_rad(J);
  const double lat = lat_deg * DEG2RAD;
  const double mu0 = cosZenith(lat, delta, H);
  if (mu0 <= 0.0) return 0.0;

  const double E0 = eccentricityCorr(J);
  return I_sc * E0 * mu0;
}

// Mean TOA horizontal irradiance at same UTC hour across the year [W/m^2]
inline double meanToaIrradiance_Wm2_sameHour(int year, int hourUTC,
                                             double lon_deg, double lat_deg) {
  const int days = isLeap(year) ? 366 : 365;
  const double lat = lat_deg * DEG2RAD;
  double sum = 0.0;
  for (int J = 1; J <= days; ++J) {
    const double EoT = equationOfTime_min(J);
    const double lst = hourUTC + (lon_deg * 4.0 + EoT) / 60.0;
    const double H = (15.0 * (lst - 12.0)) * DEG2RAD;
    const double delta = solarDeclination_rad(J);
    const double mu0 = cosZenith(lat, delta, H);
    const double E0 = eccentricityCorr(J);
    const double G0h = (mu0 > 0.0) ? (I_sc * E0 * mu0) : 0.0;
    sum += G0h;
  }
  return sum / days;
}

// ------------------ Parsing utils ------------------
static inline bool parseLine(const std::string& line, int& year, int& month,
                             int& day, int& hour, double& tempC, double& lat,
                             double& lon) {
  // Format: year;month;day;hour;temperature;latitude;longitude
  // Example: 1944;07;09;13;30.6;59.9000;17.5930
  std::stringstream ss(line);
  std::string tok;
  std::vector<std::string> tokens;
  while (std::getline(ss, tok, ';')) tokens.push_back(tok);
  if (tokens.size() != 7) return false;

  try {
    year = std::stoi(tokens[0]);
    month = std::stoi(tokens[1]);
    day = std::stoi(tokens[2]);
    hour = std::stoi(tokens[3]);
    tempC = std::stod(tokens[4]);
    lat = std::stod(tokens[5]);
    lon = std::stod(tokens[6]);
    return true;
  } catch (...) {
    return false;
  }
}

// ------------------ Main ------------------
void adjustTemps() {
  std::ios::sync_with_stdio(false);

  // Inputs
  fs::path in_dir = fs::path("datasets/Solar");
  fs::path out_file = fs::path("datasets/Solar/adjusted_temps.root");

  // ROOT output
  TFile* fout = TFile::Open(out_file.string().c_str(), "RECREATE");
  if (!fout || fout->IsZombie()) {
    std::cerr << "Failed to create ROOT file: " << out_file << "\n";
    return;
  }
  TTree* tree = new TTree("temps", "Solar-adjusted temperatures");

  // Branch variables
  int b_year, b_month, b_day, b_hour;
  double b_lat, b_lon, b_temp_raw, b_G0h, b_G0h_mean, b_correction, b_temp_adj;

  tree->Branch("year", &b_year, "year/I");
  tree->Branch("month", &b_month, "month/I");
  tree->Branch("day", &b_day, "day/I");
  tree->Branch("hour_utc", &b_hour, "hour_utc/I");
  tree->Branch("lat_deg", &b_lat, "lat_deg/D");
  tree->Branch("lon_deg", &b_lon, "lon_deg/D");
  tree->Branch("temp_raw_C", &b_temp_raw, "temp_raw_C/D");
  tree->Branch("G0h_Wm2", &b_G0h, "G0h_Wm2/D");
  tree->Branch("G0h_mean_Wm2", &b_G0h_mean, "G0h_mean_Wm2/D");
  tree->Branch("correction_C", &b_correction, "correction_C/D");
  tree->Branch("temp_adj_C", &b_temp_adj, "temp_adj_C/D");

  // Beta (°C per W/m^2). Adjust if you’ve fitted a slope.
  const double beta = 0.003;

  std::size_t total_lines = 0, bad_lines = 0, files_processed = 0;

  // Iterate files
  if (!fs::exists(in_dir) || !fs::is_directory(in_dir)) {
    std::cerr << "Input directory not found: " << in_dir << "\n";
    return;
  }

  for (auto const& dirent : fs::directory_iterator(in_dir)) {
    if (!dirent.is_regular_file()) continue;
    auto p = dirent.path();
    if (p.extension() != ".csv") continue;  // only .csv

    std::ifstream fin(p);
    if (!fin) {
      std::cerr << "Could not open: " << p << "\n";
      continue;
    }
    ++files_processed;

    std::string line;
    std::size_t line_no = 0;
    while (std::getline(fin, line)) {
      ++line_no;
      ++total_lines;
      if (line.empty()) continue;

      int year{0}, month{0}, day{0}, hour{0};
      double tempC{0.0}, lat{0.0}, lon{0.0};
      if (!parseLine(line, year, month, day, hour, tempC, lat, lon)) {
        ++bad_lines;
        continue;
      }

      // Compute irradiances
      double G0h =
          toaHorizontalIrradiance_Wm2(year, month, day, hour, lon, lat);
      double G0h_mean = meanToaIrradiance_Wm2_sameHour(year, hour, lon, lat);

      // Correction and adjusted T
      double correction = beta * (G0h - G0h_mean);
      double T_adj = tempC - correction;

      // Fill branches
      b_year = year;
      b_month = month;
      b_day = day;
      b_hour = hour;
      b_lat = lat;
      b_lon = lon;
      b_temp_raw = tempC;
      b_G0h = G0h;
      b_G0h_mean = G0h_mean;
      b_correction = correction;
      b_temp_adj = T_adj;

      tree->Fill();
    }
  }

  fout->cd();
  tree->Write();

  tree->Draw("temp_adj_C : (year + (month-1)/12.0 + (day-1)/365.2425)", "",
             "AP*");

  fout->Close();

  std::cout << "Processed files: " << files_processed << "\n";
  std::cout << "Total lines:     " << total_lines << "\n";
  std::cout << "Bad lines:       " << bad_lines << "\n";
  std::cout << "Output ROOT:     " << out_file << "\n";
}

void solar() { adjustTemps(); }
