
cd ..
./bash/clean.sh
g++ src/csv_to_root.cxx $(root-config --cflags --libs) -o csv_to_root

cd datasets/B-days
./../../bash/csv_root.sh
cd ../Climate
./../../bash/csv_root.sh
cd ../Solar
./../../bash/csv_root.sh

