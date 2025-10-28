#!/bin/bash
rm -r plots/mean_temps
rm -r plots/max_min_temps
mkdir -p plots/mean_temps
mkdir -p plots/max_min_temps

for file in ./datasets/Climate/*.root; do
    city=$(basename "$file" .root)
    echo "Analyzing $city..."
    root -l -b -q "./src/plot_mean_temp_trend.C(\"$file\", \"$city\")"
    root -l -b -q "./src/plot_max_min_trends.C(\"$file\", \"$city\")"
done

echo "All plots saved in plots/"
