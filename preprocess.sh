rm -r build/
mkdir build/
g++ src/csv_to_root.cxx $(root-config --cflags --libs) -o ./build/csv_to_root
g++ src/climate.cxx $(root-config --cflags --libs) -o ./build/climate

./bash/clean.sh

for city in datasets/clean/*.csv; do
    echo "Processing $city"
    echo "..."
    ./build/climate $(basename "$city" .csv).csv
done

./bash/csv_root.sh 

