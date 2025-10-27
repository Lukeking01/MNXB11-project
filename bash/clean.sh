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
