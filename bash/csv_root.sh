#!/bin/bash

# Loop over all CSV files in the current directory
for csv_file in ./datasets/B-days/*.csv; do
    # Skip if no CSV files exist
    [ -e "$csv_file" ] || continue

    # Extract filename without extension
    filename=$(basename "$csv_file" .csv)
    
    # Output ROOT file will have the same base name
    root_file="./datasets/B-days/${filename}.root"
    
    # Run the converter
    ./build/csv_to_root "$csv_file" "$root_file"
    
    echo "Converted $csv_file → $root_file"
done
for csv_file in ./datasets/Climate/*.csv; do
    # Skip if no CSV files exist
    [ -e "$csv_file" ] || continue

    # Extract filename without extension
    filename=$(basename "$csv_file" .csv)
    
    # Output ROOT file will have the same base name
    root_file="./datasets/Climate/${filename}.root"
    
    # Run the converter
    ./build/csv_to_root "$csv_file" "$root_file"
    
    echo "Converted $csv_file → $root_file"
done
for csv_file in ./datasets/Solar/*.csv; do
    # Skip if no CSV files exist
    [ -e "$csv_file" ] || continue

    # Extract filename without extension
    filename=$(basename "$csv_file" .csv)
    
    # Output ROOT file will have the same base name
    root_file="./datasets/Solar/${filename}.root"
    
    # Run the converter
    ./build/csv_to_root "$csv_file" "$root_file"
    
    echo "Converted $csv_file → $root_file"
done
echo "All files processed."