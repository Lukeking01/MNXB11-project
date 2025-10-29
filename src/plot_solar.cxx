// solar_monthly_norm.cpp
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "TCanvas.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1D.h"
#include "TLegend.h"
#include "TLine.h"
#include "TMultiGraph.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TTree.h"
#include "TVirtualFFT.h"

// ------------------ Calendar helpers (match your earlier logic)
// ------------------
inline bool isLeap(int y) {
  return (y % 400 == 0) || (y % 4 == 0 && y % 100 != 0);
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
// Month names for legend
static const char* kMonthName[13] = {"",    "Jan", "Feb", "Mar", "Apr",
                                     "May", "Jun", "Jul", "Aug", "Sep",
                                     "Oct", "Nov", "Dec"};

// A simple struct to accumulate mean
struct Acc {
  double sum = 0.0;
  long long n = 0;
  void add(double v) {
    sum += v;
    ++n;
  }
  double mean() const {
    return (n > 0) ? (sum / n) : std::numeric_limits<double>::quiet_NaN();
  }
};

int solarMonthlyNorm() {
  gROOT->SetBatch(kTRUE);  // no GUI popups

  const std::string in_path = "datasets/Solar/adjusted_temps.root";
  TFile* fin = TFile::Open(in_path.c_str(), "READ");
  if (!fin || fin->IsZombie()) {
    std::cerr << "ERROR: Cannot open ROOT file: " << in_path << "\n";
    return 1;
  }

  TTree* tree = dynamic_cast<TTree*>(fin->Get("temps"));
  if (!tree) {
    std::cerr << "ERROR: Could not find TTree 'temps' in file.\n";
    fin->Close();
    return 1;
  }

  // Bind branches (structure taken from your producer)
  int year, month, day, hour;
  double lat, lon, temp_raw, G0h, G0h_mean, correction, temp_adj;

  tree->SetBranchAddress("year", &year);
  tree->SetBranchAddress("month", &month);
  tree->SetBranchAddress("day", &day);
  tree->SetBranchAddress("hour_utc", &hour);
  tree->SetBranchAddress("lat_deg", &lat);
  tree->SetBranchAddress("lon_deg", &lon);
  tree->SetBranchAddress("temp_raw_C", &temp_raw);
  tree->SetBranchAddress("G0h_Wm2", &G0h);
  tree->SetBranchAddress("G0h_mean_Wm2", &G0h_mean);
  tree->SetBranchAddress("correction_C", &correction);
  tree->SetBranchAddress("temp_adj_C", &temp_adj);

  const Long64_t N = tree->GetEntries();
  if (N <= 0) {
    std::cerr << "No entries in tree.\n";
    fin->Close();
    return 0;
  }

  // ---------- PASS 1: per-day-of-year min/max of adjusted temperatures
  // ---------- Index day-of-year as [1..366]. We'll ignore index 0.
  const int MAX_DOY = 366;
  std::vector<double> doy_min(MAX_DOY + 1,
                              std::numeric_limits<double>::infinity());
  std::vector<double> doy_max(MAX_DOY + 1,
                              -std::numeric_limits<double>::infinity());
  std::vector<long long> doy_cnt(MAX_DOY + 1, 0);

  for (Long64_t i = 0; i < N; ++i) {
    tree->GetEntry(i);
    const int J = dayOfYear(year, month, day);
    if (J < 1 || J > MAX_DOY) continue;
    doy_min[J] = std::min(doy_min[J], temp_adj);
    doy_max[J] = std::max(doy_max[J], temp_adj);
    ++doy_cnt[J];
  }

  // ---------- PASS 2: normalize each entry by its day-of-year, then monthly
  // means ---------- Key: (year, month) -> accumulator of normalized values
  std::map<std::pair<int, int>, Acc> monthly_means;
  std::set<int> years_present;  // to build time axis

  for (Long64_t i = 0; i < N; ++i) {
    tree->GetEntry(i);
    const int J = dayOfYear(year, month, day);
    if (J < 1 || J > MAX_DOY) continue;

    const double lo = doy_min[J];
    const double hi = doy_max[J];
    if (!std::isfinite(lo) || !std::isfinite(hi)) continue;

    double norm = 0.5;  // fallback if degenerate
    if (hi > lo) {
      norm = (temp_adj - lo) / (hi - lo);  // normalize to [0,1] for this DOY
      // Clamp for numerical safety
      if (norm < 0.0) norm = 0.0;
      if (norm > 1.0) norm = 1.0;
    }

    monthly_means[{year, month}].add(norm);
    years_present.insert(year);
  }

  fin->Close();

  if (monthly_means.empty()) {
    std::cerr << "No monthly data accumulated (check input branches?).\n";
    return 1;
  }

  // ---------- Build TGraphs: one per month, x = year, y = avg normalized
  // ---------- Collect and sort years
  std::vector<int> years_sorted(years_present.begin(), years_present.end());
  std::sort(years_sorted.begin(), years_sorted.end());

  // For each month, collect (year, value) if present
  std::vector<TGraph*> graphs(13, nullptr);  // index 1..12
  int color_cycle[12] = {kRed + 1,    kBlue + 1, kGreen + 2,  kMagenta + 1,
                         kOrange + 1, kCyan + 2, kViolet + 1, kAzure + 2,
                         kTeal + 3,   kPink + 2, kSpring + 5, kGray + 2};
  int marker_cycle[12] = {20, 21, 22, 23, 24, 25, 26, 20, 21, 22, 23, 24};

  for (int m = 1; m <= 12; ++m) {
    std::vector<double> xs, ys;
    xs.reserve(years_sorted.size());
    ys.reserve(years_sorted.size());

    for (int y : years_sorted) {
      auto it = monthly_means.find({y, m});
      if (it == monthly_means.end() || it->second.n == 0) continue;
      xs.push_back(static_cast<double>(y));
      ys.push_back(it->second.mean());
    }

    if (!xs.empty()) {
      graphs[m] = new TGraph(static_cast<int>(xs.size()), xs.data(), ys.data());
      graphs[m]->SetName(Form("g_%02d", m));
      graphs[m]->SetTitle(kMonthName[m]);
      graphs[m]->SetMarkerStyle(marker_cycle[m - 1]);
      graphs[m]->SetMarkerSize(0.8);
      graphs[m]->SetLineWidth(2);
      graphs[m]->SetLineColor(color_cycle[m - 1]);
      graphs[m]->SetMarkerColor(color_cycle[m - 1]);
    }
  }

  // ---------- Draw all months on one canvas ----------
  TCanvas* c = new TCanvas(
      "c_monthly_norm", "Monthly normalized adjusted temperature", 1200, 700);
  gStyle->SetOptStat(0);

  TMultiGraph* mg = new TMultiGraph();
  for (int m = 1; m <= 12; ++m) {
    if (graphs[m]) mg->Add(graphs[m], "LP");
  }

  mg->SetTitle(
      "Monthly Mean of Day-of-Year Normalized Adjusted "
      "Temperature;Year;Normalized mean (0..1)");
  mg->Draw("A");

  // Force y-axis to [0,1]
  mg->GetYaxis()->SetRangeUser(0.0, 1.0);

  // Make legend bigger and clearer, placed below x-axis labels
  TLegend* leg = new TLegend(0.1, 0.01, 0.9, 0.12);  // shifted down
  leg->SetNColumns(6);  // split months into 6 columns
  leg->SetBorderSize(0);
  leg->SetFillStyle(0);
  leg->SetTextSize(0.035);
  for (int m = 1; m <= 12; ++m) {
    if (graphs[m]) leg->AddEntry(graphs[m], kMonthName[m], "lp");
  }
  leg->Draw();

  // Grid and save
  gPad->SetGrid();
  c->SaveAs("plots/solar/monthly_norm_temp.png");

  // Also save to a ROOT file with the graphs
  TFile* fout =
      TFile::Open("datasets/Solar/monthly_norm_temp.root", "RECREATE");
  if (fout && !fout->IsZombie()) {
    c->Write("canvas_monthly_norm");
    mg->Write("multigraph_monthly_norm");
    for (int m = 1; m <= 12; ++m) {
      if (graphs[m]) graphs[m]->Write();
    }
    fout->Close();
  } else {
    std::cerr << "WARNING: Could not create output ROOT file for graphs.\n";
  }

  std::cout << "Wrote plots to plots/solar/monthly_norm_temp.png\n";
  std::cout << "Wrote graphs to datasets/Solar/monthly_norm_temp.root\n";

  // ---------- Single timeline plot: months in succession across years
  // ----------
  std::vector<std::pair<double, double>> timeline;
  timeline.reserve(monthly_means.size());
  for (int y : years_sorted) {
    for (int m = 1; m <= 12; ++m) {
      auto it = monthly_means.find({y, m});
      if (it == monthly_means.end() || it->second.n == 0) continue;
      double x = static_cast<double>(y) + (m - 1) / 12.0;
      double v = it->second.mean();
      timeline.emplace_back(x, v);
    }
  }

  if (!timeline.empty()) {
    std::sort(timeline.begin(), timeline.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    std::vector<double> xs, ys;
    xs.reserve(timeline.size());
    ys.reserve(timeline.size());
    for (auto& p : timeline) {
      xs.push_back(p.first);
      ys.push_back(p.second);
    }

    // Clean, white background
    gStyle->SetOptStat(0);
    gStyle->SetCanvasColor(0);
    gStyle->SetPadColor(0);
    gStyle->SetFrameFillColor(0);
    gStyle->SetTitleFillColor(0);
    gStyle->SetStatColor(0);

    TCanvas* c2 = new TCanvas(
        "c_monthly_norm_timeline",
        "Monthly normalized adjusted temperature (timeline)", 1200, 700);
    c2->SetBottomMargin(0.13);
    c2->SetLeftMargin(0.10);
    c2->SetRightMargin(0.03);
    c2->SetTopMargin(0.06);

    TGraph* g_tl =
        new TGraph(static_cast<int>(xs.size()), xs.data(), ys.data());
    g_tl->SetName("g_monthly_norm_timeline");
    g_tl->SetTitle(
        "Monthly Mean of Day-of-Year Normalized Adjusted Temperature;Year "
        "(months in succession);Normalized mean (0..1)");
    g_tl->SetLineWidth(2);
    g_tl->SetMarkerStyle(20);
    g_tl->SetMarkerSize(0.6);

    g_tl->Draw("ALP");
    g_tl->GetYaxis()->SetRangeUser(0.0, 1.0);

    // Fewer x divisions (major only)
    int y_min = years_sorted.front();
    int y_max = years_sorted.back();
    g_tl->GetXaxis()->SetNdivisions(
        510, /*optimize=*/true);  // ROOT default, cleaner
    // Or: g_tl->GetXaxis()->SetNdivisions(8, true); // ~8 major ticks total

    // Grid: horizontal only, light
    c2->SetGridx(0);
    c2->SetGridy(1);
    gPad->SetGridy(1);
    gPad->SetGridx(0);
    gStyle->SetGridColor(kGray + 2);
    gStyle->SetGridStyle(3);
    gStyle->SetGridWidth(1);

    // Vertical guides every 5 years (lighter than data)
    int step = (y_max - y_min <= 60) ? 5 : 10;
    for (int yr = ((y_min / step) * step); yr <= y_max; yr += step) {
      TLine* L = new TLine(yr, 0.0, yr, 1.0);
      L->SetLineStyle(7);          // dashed
      L->SetLineColor(kGray + 1);  // light
      L->SetLineWidth(1);
      L->Draw("same");
    }

    c2->SaveAs("plots/solar/monthly_norm_temp_timeline.png");

    // Save into the ROOT output as well
    TFile* fout2 =
        TFile::Open("datasets/Solar/monthly_norm_temp.root", "UPDATE");
    if (fout2 && !fout2->IsZombie()) {
      c2->Write("canvas_monthly_norm_timeline");
      g_tl->Write();
      fout2->Close();
    } else {
      std::cerr << "WARNING: Could not update monthly_norm_temp.root with "
                   "timeline.\n";
    }
  } else {
    std::cerr << "No (year,month) points for the timeline plot.\n";
  }

  // ---------- Frequency analysis ----------
  {
    // Build uniform monthly series (year + month → index)
    int y_min = years_sorted.front();
    int y_max = years_sorted.back();
    int n_months = (y_max - y_min + 1) * 12;

    std::vector<double> series(n_months, 0.0);
    std::vector<int> counts(n_months, 0);

    for (auto& kv : monthly_means) {
      int y = kv.first.first;
      int m = kv.first.second;
      double mean = kv.second.mean();
      int idx = (y - y_min) * 12 + (m - 1);
      series[idx] += mean;
      counts[idx] += 1;
    }
    for (int i = 0; i < n_months; ++i) {
      if (counts[i] > 0)
        series[i] /= counts[i];
      else
        series[i] = 0.0;  // gap → 0
    }

    // Detrend (remove mean)
    double mean_val = 0.0;
    for (double v : series) mean_val += v;
    mean_val /= series.size();
    for (double& v : series) v -= mean_val;

    int N = series.size();

    // FFT
    TVirtualFFT* fft = TVirtualFFT::FFT(1, &N, "R2C EX K");
    fft->SetPoints(&series[0]);
    fft->Transform();

    // Magnitude spectrum
    int n_freq = N / 2;
    double nyquist = 12.0 / 2.0;  // cycles per year, Nyquist freq = 6
    TH1D* h_pow =
        new TH1D("h_pow", "Frequency Spectrum;Frequency [cycles/year];Power",
                 n_freq, 0, nyquist);

    double re, im;
    for (int i = 0; i < n_freq; ++i) {
      fft->GetPointComplex(i, re, im);
      double mag2 = re * re + im * im;
      h_pow->SetBinContent(i + 1, mag2);
    }

    TCanvas* c3 = new TCanvas("c_freq", "Frequency analysis", 1000, 600);
    gPad->SetLogy();
    h_pow->GetXaxis()->SetRangeUser(0, 1.0);  // zoom to 0–1 cpy
    h_pow->SetLineColor(kBlue + 1);
    h_pow->SetLineWidth(2);
    h_pow->Draw("HIST");

    c3->SaveAs("plots/solar/solar_frequency_analysis.png");

    // ---------- Periodogram (zoomed 2–20 years) ----------
    TH1D* h_per = new TH1D("h_per", "Periodogram;Period [years];Power", 500,
                           0.5, 50.0);  // fine bins up to 50 yrs

    for (int i = 1; i <= n_freq; ++i) {
      double f = (i - 1) * 12.0 / N;  // cycles/year
      if (f > 0) {
        double T = 1.0 / f;  // years
        if (T >= 0.5 && T <= 50.0) {
          double power = h_pow->GetBinContent(i);
          h_per->Fill(T, power);
        }
      }
    }

    TCanvas* c4 =
        new TCanvas("c_period", "Period analysis (2–20 yrs)", 1000, 600);
    gPad->SetLogy();
    h_per->GetXaxis()->SetRangeUser(2.0, 20.0);  // zoom to 2–20 years
    h_per->SetLineColor(kRed + 1);
    h_per->SetLineWidth(2);
    h_per->Draw("HIST");
    c4->SaveAs("plots/solar/period_analysis.png");

    TFile* fout3 =
        TFile::Open("datasets/Solar/monthly_norm_temp.root", "UPDATE");
    if (fout3 && !fout3->IsZombie()) {
      h_pow->Write();
      c3->Write("canvas_frequency_analysis");
      fout3->Close();
    }
  }
  return 0;
}

void plotTempOverTime() {
  // open the root file
  TFile* f = TFile::Open("datasets/Solar/adjusted_temps.root");
  TTree* t = (TTree*)f->Get("temps");  // adjust to your TTree name

  // variables for branches
  int year, month, day;
  double temp;

  t->SetBranchAddress("year", &year);
  t->SetBranchAddress("month", &month);
  t->SetBranchAddress("day", &day);
  t->SetBranchAddress("temp_adj_C", &temp);

  // create graph
  int nEntries = t->GetEntries();
  auto gr = new TGraph(nEntries);

  for (int i = 0; i < nEntries; i++) {
    t->GetEntry(i);

    // fractional year: year + (month-1)/12 + (day-1)/365
    double fractionalYear = year + (month - 1) / 12.0 + (day - 1) / 365.0;

    gr->SetPoint(i, fractionalYear, temp);
  }

  gr->SetTitle("Temperature over Time;Year;Temperature");
  gr->SetMarkerStyle(20);
  gr->SaveAs("plots/solar/TempOverTime.png");
  gr->Draw("AP");
}

void plot_solar() {
  plotTempOverTime();
  solarMonthlyNorm();
}
