#!/bin/bash
mkdir -p plots/solar
root -l -b -q 'src/solar.cxx+'
root -l -b -q 'src/plot_solar.cxx+'
