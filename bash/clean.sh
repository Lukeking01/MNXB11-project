rm -r datasets/

mkdir datasets/
mkdir datasets/raw/
mkdir datasets/clean/
mkdir datasets/Solar/
mkdir datasets/B-days/
mkdir datasets/Climate/

cp raw/datasets.tgz datasets/raw/datasets.tgz
cd datasets/raw

tar zxvf datasets.tgz
rm *.dat
rm *.txt

cd ..
for f in raw/*.csv; do
  base="${f##*_}"       
  out="${base%.csv}.csv" 
  cp $f clean/$out
done
cd clean/

for f in *.csv; do
  wc -l $f
awk -F';' '
    BEGIN { OFS=";"; lat=""; lon=""; }

    # Capture lat/lon from the metadata line:
    # "YYYY-MM-DD HH:MM:SS;YYYY-MM-DD HH:MM:SS;H;LAT;LON"
    $1 ~ /^[0-9]{4}-[0-9]{2}-[0-9]{2}[[:space:]][0-9]{2}:[0-9]{2}:[0-9]{2}$/ &&
    $2 ~ /^[0-9]{4}-[0-9]{2}-[0-9]{2}[[:space:]][0-9]{2}:[0-9]{2}:[0-9]{2}$/ &&
    $4 ~ /^-?[0-9]+([.][0-9]+)?$/ &&
    $5 ~ /^-?[0-9]+([.][0-9]+)?$/ {
      lat = $4; lon = $5;
      next
    }

    # Process actual data lines: "YYYY-MM-DD;HH:MM:SS;value;code"
    $1 ~ /^[0-9]{4}-[0-9]{2}-[0-9]{2}$/ && $2 ~ /^[0-9]{2}:[0-9]{2}:[0-9]{2}$/ {
      c = $4
      gsub(/^[ \t"]+|[ \t"]+$/, "", c)       # trim spaces/quotes around code

      if (c == "G") {
        split($1, d, "-")                    # date -> year,month,day
        split($2, t, ":")                    # time -> hour
        gsub(/^0+/, "", t[1])                # strip leading zeros in hour
        if (t[1] == "") t[1] = 0
        print d[1], d[2], d[3], t[1], $3, c, lat, lon
      }
    }
  ' "$f" > tmp && mv tmp "$f"
  wc -l "$f"
done
cd ..
cd ..



cd datasets
for f in clean/*.csv; do
  original_lines=$(wc -l < "$f")
  filename=$(basename "$f")
  output_file="B-days/$filename"
  awk -F';' '
   BEGIN { OFS=";"}

    ($2 == "11" && $3 == "06") ||
    ($2 == "04" && $3 == "12") || 
    ($2 == "03" && $3 == "11") { print $0 }
  ' "$f" > "$output_file"
  
  filtered_lines=$(wc -l < "$output_file")
  echo "Lines: $original_lines → $filtered_lines"
  echo "Saved to: $output_file"
  echo "---"
done


start_hour="11"
stop_hour="15"
for f in clean/*.csv; do
  echo "Processing $f - hours ${start_hour}-${stop_hour}"
  original_lines=$(wc -l < "$f")
  filename=$(basename "$f")
  output_file="Solar/$filename"
  awk -F';' -v start="$start_hour" -v end="$stop_hour" '
    BEGIN { OFS=";" }
    $4 >= start && $4 <= end
  ' "$f" > "$output_file"
  
  filtered_lines=$(wc -l < "$output_file")
  echo "Lines: $original_lines → $filtered_lines"
  echo "Saved to: $output_file"
  echo "---"
done

