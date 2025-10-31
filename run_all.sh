rm -r plots/
mkdir plots/

# Compiles .cxx files , cleans and structures data
chmod +x ./preprocess.sh
./preprocess.sh

# Executes the solar analysis and generates plots for it
chmod +x ./bash/solar_analysis.sh
./bash/solar_analysis.sh

# Executes the climate analysis and generates plots for it
chmod +x ./bash/climate_analysis.sh
./bash/climate_analysis.sh


# Executes the bday analysis and generates plots for it
chmod +x ./bash/bdays.sh
./bash/bdays.sh

cd tex
pdflatex main.tex
pdflatex main.tex
pdflatex main.tex
rm -f *.aux *.log *.toc *.dvi *.fls *.fdb_latexmk *.out *.out.ps *.bbl *.blg
mv main.pdf ../MNXB11-project.pdf