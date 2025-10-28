#include <cmath>
#include <iostream>

namespace solar_std {

// Constants
constexpr double PI = 3.14159265358979323846;
constexpr double DEG2RAD = PI / 180.0;
constexpr double RAD2DEG = 180.0 / PI;
constexpr double I_sc = 1367.0;  // Solar constant [W/m^2]

// Leap year check
inline bool isLeap(int y) {
  return (y % 400 == 0) || (y % 4 == 0 && y % 100 != 0);
}

// Day of year (1..365/366)
inline int dayOfYear(int y, int m, int d) {
  static const int cum[12] = {0,   31,  59,  90,  120, 151,
                              181, 212, 243, 273, 304, 334};
  int J = cum[m - 1] + d;
  if (m > 2 && isLeap(y)) ++J;
  return J;
}

// Equation of Time (minutes). Spencer/FAO-style approximation.
inline double equationOfTime_min(int J) {
  // B in radians
  double B = 2.0 * PI * (J - 81) / 364.0;
  return 9.87 * std::sin(2.0 * B) - 7.53 * std::cos(B) - 1.5 * std::sin(B);
}

// Solar declination (radians)
inline double solarDeclination_rad(int J) {
  return 23.45 * DEG2RAD * std::sin(2.0 * PI * (284.0 + J) / 365.0);
}

// Eccentricity correction factor E0 for Earth-Sun distance
inline double eccentricityCorr(int J) {
  return 1.0 + 0.033 * std::cos(2.0 * PI * J / 365.0);
}

// Compute local solar time (hours) from local clock time, longitude, and
// timezone tz_offset_hours: e.g., +1 for CET, +2 for CEST, 0 for UTC
inline double localSolarTime_hours(int year, int month, int day, int hour,
                                   int minute, double longitude_deg,
                                   double tz_offset_hours) {
  int J = dayOfYear(year, month, day);
  double L_st = 15.0 * tz_offset_hours;  // standard meridian [deg]
  double EoT = equationOfTime_min(J);    // minutes
  double clock = hour + minute / 60.0;   // local civil time [h]
  // LST = clock + (4*(L_st - lon) + EoT)/60
  return clock + (4.0 * (L_st - longitude_deg) + EoT) / 60.0;
}

// Cosine of solar zenith angle
inline double cosZenith(double lat_rad, double decl_rad,
                        double hour_angle_rad) {
  return std::sin(lat_rad) * std::sin(decl_rad) +
         std::cos(lat_rad) * std::cos(decl_rad) * std::cos(hour_angle_rad);
}

// Instantaneous TOA irradiance on horizontal surface [W/m^2]
// at the given local solar time. Nighttime returns 0.
inline double toaHorizontalIrradiance_Wm2(int year, int month, int day,
                                          int hour, int minute,
                                          double longitude_deg,
                                          double latitude_deg,
                                          double tz_offset_hours) {
  int J = dayOfYear(year, month, day);
  double lst = localSolarTime_hours(year, month, day, hour, minute,
                                    longitude_deg, tz_offset_hours);
  double H_deg = 15.0 * (lst - 12.0);  // hour angle [deg]
  double H = H_deg * DEG2RAD;
  double delta = solarDeclination_rad(J);
  double lat = latitude_deg * DEG2RAD;

  double mu0 = cosZenith(lat, delta, H);  // cos(theta_z)
  if (mu0 <= 0.0) return 0.0;             // sun below horizon
  double E0 = eccentricityCorr(J);
  double G0n = I_sc * E0;  // normal TOA irradiance
  return G0n * mu0;        // horizontal plane
}

// Annual mean of TOA horizontal irradiance at the same local clock time
inline double meanToaHorizontalIrradiance_Wm2_sameClockTime(
    int year, int hour, int minute, double longitude_deg, double latitude_deg,
    double tz_offset_hours) {
  int days = isLeap(year) ? 366 : 365;
  double sum = 0.0;
  for (int J = 1; J <= days; ++J) {
    // Convert J back to a calendar date is overkill: we only need J.
    // We can reuse formulas by faking month/day where J matches.
    // Instead, call a helper using J directly:
    double lst;
    {
      double L_st = 15.0 * tz_offset_hours;
      double EoT = equationOfTime_min(J);
      double clock = hour + minute / 60.0;
      lst = clock + (4.0 * (L_st - longitude_deg) + EoT) / 60.0;
    }
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

// --- Public API ---
// Returns solar-standardized temperature T_adj (°C).
// Inputs:
//  - year, month, day, hour, minute : local civil time
//  - longitude_deg (east +), latitude_deg (north +)
//  - tz_offset_hours : timezone offset from UTC (e.g. CET=+1, CEST=+2)
//  - temperature_C : observed temperature (°C)
//  - beta_degC_per_Wm2 : °C per (W/m^2) sensitivity; tune from your data.
//                        Typical starting value ~0.003
inline double solarAdjustedTemperature(int year, int month, int day, int hour,
                                       int minute, double longitude_deg,
                                       double latitude_deg,
                                       double tz_offset_hours,
                                       double temperature_C,
                                       double beta_degC_per_Wm2 = 0.003) {
  double G0h =
      toaHorizontalIrradiance_Wm2(year, month, day, hour, minute, longitude_deg,
                                  latitude_deg, tz_offset_hours);
  double G0h_mean = meanToaHorizontalIrradiance_Wm2_sameClockTime(
      year, hour, minute, longitude_deg, latitude_deg, tz_offset_hours);
  // Remove linear contribution of (instantaneous - annual mean at same clock
  // time)
  return temperature_C - beta_degC_per_Wm2 * (G0h - G0h_mean);
}

}  // namespace solar_std

// --- Example usage ---
int main() {
  using namespace solar_std;

  // Berlin, Oct 28, 2025, 15:30 local, CEST is over by then -> CET = +1
  int year = 1958, month = 6, day = 21, hour = 12, minute = 00;
  double lon = 12.9468;  // deg (east +)
  double lat = 57.7607;  // deg (north +)
  double tz = 0.0;      // CET
  double T = 17.0;      // °C measured

  double T_adj =
      solarAdjustedTemperature(year, month, day, hour, minute, lon, lat, tz, T);
  std::cout << "Solar-standardized temperature: " << T_adj << " °C\n";
  return 0;
}
// 1884;01;09;13;4.7;57.7607;12.9468
// 1958;06;21;12;17.0;57.7607;12.9468