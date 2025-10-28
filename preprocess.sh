rm -r build/
mkdir build/
g++ src/csv_to_root.cxx $(root-config --cflags --libs) -o ./build/csv_to_root
g++ src/climate.cxx $(root-config --cflags --libs) -o ./build/climate
g++ src/sweden_average.cxx $(root-config --cflags --libs) -o ./build/sweden_average

./bash/clean.sh

for city in datasets/clean/*.csv; do
    echo "Processing $city"
    echo "..."
    ./build/climate $(basename "$city" .csv).csv
done

# Remove Halmstad
rm ./datasets/Climate/Halmstad.csv
./build/sweden_average
./bash/csv_root.sh 

rm ./datasets/Climate/*.csv