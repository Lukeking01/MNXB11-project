#!/bin/bash

# Loop over all CSV files in the current directory
for csv_file in ./*.csv; do
    # Skip if no CSV files exist
    [ -e "$csv_file" ] || continue

    # Extract filename without extension
    filename=$(basename "$csv_file" .csv)
    
    # Output ROOT file will have the same base name
    root_file="./${filename}.root"
    
    # Run the converter
    ./../../src/csv_to_root "$csv_file" "$root_file"
    
    echo "Converted $csv_file → $root_file"
done

echo "All files processed."