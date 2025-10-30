# Climate Analysis – Long-Term Temperature Trends in Sweden

## Overview
This project analyses long-term temperature data from the **Swedish Meteorological and Hydrological Institute (SMHI)** to investigate trends and patterns in Swedish climate from around **1850 to 2024**.  
The analysis focuses on three main questions:
1. **Long-term trends:** How have average temperatures in Sweden evolved over time?  
2. **Birthday trends:** How have temperatures changed on specific calendar dates (our birthdays) throughout the dataset?  
3. **Solar events:** Are there any correlations between temperature variations and solar phenomena?

All data processing, analysis, and plotting are performed using **C++**, **ROOT**, and **Bash scripts**.

---

## Dataset
- **Source:** SMHI open climate data  
- **Format:** CSV files containing temperature measurements from multiple Swedish weather stations  
- **Time span:** ~1850 – 2024  
- **Variables:** Daily temperature values (°C), timestamps, and station metadata  

CSV data are converted into ROOT files for efficient analysis.

---

## Methods & Tools
- **C++** – Core data analysis and statistical calculations  
- **ROOT** – Data handling, histogramming, fitting, and visualization  
- **Bash** – Automation of preprocessing and execution steps  

The workflow includes:
1. Preprocessing of raw CSV files  
2. Conversion to ROOT format  
3. Statistical analysis (mean temperature, anomalies, trends)  
4. Plot generation for each research question  

---

## Repository Structure
    MNXB11-project/

        ├── bash/ # Bash scripts for automation
        ├── build/ # contains the built .c++ files
        ├── datasets/
            ├── B-days #
            ├── clean #
            ├── Climate #
            ├── raw #
            ├── Solar #
        ├── raw/ # Raw unprocessed compressed climate data
        ├── plots/ # Generated plots and results
        ├── src/ # C++ source files
        ├── preprocess.sh
        ├── run_all.sh
        ├── tex
            ├── all .tex files
        └── README.md



---

## How to Run

1. Ensure that **ROOT** is installed and available in your environment.  
2. Run the main bash script to execute the full pipeline:

```bash
./run_project.sh
# (or the script name defined in scripts/)
```

This will:

- Preprocess and convert the CSV data  
- Compile the C++ code  
- Perform the analyses  
- Generate plots in the `output/` folder  

---

## Results

The analysis produces:

- Long-term temperature trend plots for some Swedish stations  
- Temperature evolution on specific dates (e.g., birthdays)  
- Visualizations exploring temperature–solar correlations  

> Results are saved as image files and can be directly used in reports or presentations.

---

## Notes

- The dataset is large; ensure sufficient memory and disk space.   
- All analysis parameters and output paths can be adjusted in the corresponding Bash and C++ files.
