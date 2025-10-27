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
    BEGIN { OFS=";" }
    # Process only lines whose first field looks like YYYY-MM-DD
    $1 ~ /^[0-9]{4}-[0-9]{2}-[0-9]{2}$/ {
      # sanitize code (col 4)
      c = $4
      gsub(/^[ \t"]+|[ \t"]+$/, "", c)

      if (c == "G") {
        split($1, d, "-")         # date -> d[1]=year, d[2]=month, d[3]=day
        split($2, t, ":")         # time -> t[1]=hour
        gsub(/^0+/, "", t[1])     # remove leading zero(s) in hour
        if (t[1] == "") t[1] = 0  # safety if hour was "00"
        print d[1], d[2], d[3], t[1], $3
      }
    }
  ' "$f" > tmp && mv tmp "$f"
  wc -l "$f"
done
cd ..
cd ..
