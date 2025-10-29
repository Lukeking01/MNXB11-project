#!/bin/bash

city=$(basename "$file" .root)
    echo "Analyzing $city..."
    root -l "./src/plot_bdays.C(\"datasets/B-days/Lund_points.csv\", \"Lund\")"

