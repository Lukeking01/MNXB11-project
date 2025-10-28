#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <tuple>

namespace solar_std {

constexpr double PI = 3.14159265358979323846;
constexpr double DEG2RAD = PI / 180.0;
constexpr double I_sc = 1367.0;  // W/m^2

inline bool isLeap(int y) {
  return (y % 400 == 0) || (y % 4 == 0 && y % 100 != 0);
}

inline int dayOfYear(int y, int m, int d) {
  if (m < 1 || m > 12) throw std::invalid_argument("month must be 1..12");
  static const int cum[12] = {0,   31,  59,  90,  120, 151,
                              181, 212, 243, 273, 304, 334};
  int maxd = (m == 2 ? (isLeap(y) ? 29 : 28)
                     : (m == 4 || m == 6 || m == 9 || m == 11 ? 30 : 31));
  if (d < 1 || d > maxd)
    throw std::invalid_argument("day out of range for month");
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
inline double toaHorizontalIrradiance_Wm2(double year, double month, double day,
                                          double hourUTC, double longitude_deg,
                                          double latitude_deg) {
  // Basic sanity checks (you can relax these if needed)
  if (hourUTC < 0.0 || hourUTC > 23.999)
    throw std::invalid_argument("hourUTC must be 0..23");
  if (latitude_deg < -90.0 || latitude_deg > 90.0)
    throw std::invalid_argument("latitude must be in degrees [-90,90]");
  if (longitude_deg < -180.0 || longitude_deg > 180.0)
    throw std::invalid_argument("longitude must be in degrees [-180,180]");

  int y = (int)std::round(year);
  int m = (int)std::round(month);
  int d = (int)std::round(day);
  int J = dayOfYear(y, m, d);

  double EoT = equationOfTime_min(J);
  // Convert UTC to local solar time using longitude (deg east positive)
  double lst = hourUTC + (longitude_deg * 4.0 + EoT) / 60.0;
  double H = (15.0 * (lst - 12.0)) * DEG2RAD;

  double delta = solarDeclination_rad(J);
  double lat = latitude_deg * DEG2RAD;

  double mu0 = cosZenith(lat, delta, H);
  if (!(mu0 > 0.0)) return 0.0;

  double E0 = eccentricityCorr(J);
  return I_sc * E0 * mu0;  // W/m^2
}

// Mean TOA horizontal irradiance for same UTC hour across the year [W/m^2]
inline double meanToaIrradiance_Wm2_sameHour(int year, double hourUTC,
                                             double longitude_deg,
                                             double latitude_deg) {
  if (hourUTC < 0.0 || hourUTC > 23.999)
    throw std::invalid_argument("hourUTC must be 0..23");
  int days = isLeap(year) ? 366 : 365;
  double sum = 0.0;
  for (int J = 1; J <= days; ++J) {
    double EoT = equationOfTime_min(J);
    double lst = hourUTC + (longitude_deg * 4.0 + EoT) / 60.0;
    double H = (15.0 * (lst - 12.0)) * DEG2RAD;
    double delta = solarDeclination_rad(J);
    double lat = latitude_deg * DEG2RAD;
    double mu0 = cosZenith(lat, delta, H);
    double E0 = eccentricityCorr(J);
    double G0h = (mu0 > 0.0) ? (I_sc * E0 * mu0) : 0.0;
    sum += G0h;
  }
  return sum / days;
}

// Main: returns (T_adj, G0h, G0h_mean, correction)
inline std::tuple<double, double, double, double> solarAdjustedTemperature(
    double year, double month, double day, double hourUTC, double longitude_deg,
    double latitude_deg, double temperature_C, double beta_degC_per_Wm2 = 0.003,
    double max_abs_correction_C = 20.0)  // safety cap
{
  if (!(beta_degC_per_Wm2 > 0.0 && beta_degC_per_Wm2 < 1.0))
    throw std::invalid_argument("beta should be in °C per W/m^2; try 0.003");

  int y = (int)std::round(year);

  double G0h = toaHorizontalIrradiance_Wm2(year, month, day, hourUTC,
                                           longitude_deg, latitude_deg);
  double G0h_mean =
      meanToaIrradiance_Wm2_sameHour(y, hourUTC, longitude_deg, latitude_deg);

  double correction = beta_degC_per_Wm2 * (G0h - G0h_mean);

  // Optional cap to prevent wild outputs if inputs are off
  if (max_abs_correction_C > 0.0)
    correction =
        std::clamp(correction, -max_abs_correction_C, max_abs_correction_C);

  double T_adj = temperature_C - correction;
  return {T_adj, G0h, G0h_mean, correction};
}

}  // namespace solar_std

// --- Example usage ---
int main() {
  using namespace solar_std;

  // Berlin, Oct 28, 2025, 15:30 local, CEST is over by then -> CET = +1
  double year = 1958, month = 6, day = 21, hour = 12;
  double lon = 12.9468;  // deg (east +)
  double lat = 57.7607;  // deg (north +)
  double tz = 0.0;       // CET
  double T = 17.0;       // °C measured

  auto [T_adj, G0h, G0h_mean, corr] =
      solarAdjustedTemperature(year, month, day, hour, lon, lat, T);
  std::cout << "G0h        = " << G0h << " W/m^2\n";
  std::cout << "G0h_mean   = " << G0h_mean << " W/m^2\n";
  std::cout << "correction = " << corr << " °C\n";
  std::cout << "T_adj      = " << T_adj << " °C\n";
  return 0;
}
// 1884;01;09;13;4.7;57.7607;12.9468
// 1958;06;21;12;17.0;57.7607;12.9468