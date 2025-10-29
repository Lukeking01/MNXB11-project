#!/bin/bash
rm -r plots/bdays
mkdir plots/bdays/
for file in ./datasets/B-days/*.csv; do
    city=$(basename "$file" .csv)
    echo "Analyzing $city..."

    out_file="./datasets/B-days/${city}_points.csv"

    ./build/b-days "$file" "$out_file"
    root -l -b -q "./src/plot_bdays.C(\"$out_file\", \"$city\")"
done

