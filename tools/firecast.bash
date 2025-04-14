#!/bin/sh
#SBATCH -J FC$2
#SBATCH -n 16
#SBATCH --partition=intel
#SBATCH --time=02:00:00
#SBATCH --mail-user=filippi_j@univ-corse.fr

#First include the config file

# Check number of arguments otherwise print a usage message
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 TEMPLATE_DIR CASE_NAME [OUTPUT_DIR]"
    exit 1
fi
source config_firecast_local.bash
# Detect sed in-place flag for portability
if sed --version >/dev/null 2>&1; then
    # GNU sed
    SED_INPLACE=(-i)
else
    # BSD sed (macOS)
    SED_INPLACE=(-i '')
fi
#This scripts takes 3 arguments, first is the template directory, second is the target case, third iis optional, a target case path if it is given it uses that instead

TEMPLATE_DIR="$1"
FF_UNCOUPLED_CASE="$UNCOUPLED_CASE_PATH/$2"
OUTPUT_DIR="$COUPLED_CASE_PATH/$2"

if [ -n "$3" ]; then
    OUTPUT_DIR="$3"
fi


if [ ! -d "$OUTPUT_DIR" ]; then
    mkdir -p "$OUTPUT_DIR" || { echo "Failed to create output directory: $OUTPUT_DIR"; exit 1; }
fi

# Copy the config directory to the output directory
#cp -r "$FF_UNCOUPLED_CASE" "$OUTPUT_DIR/ForeFire" || { echo "Failed to copy FF_UNCOUPLED_FILE file to $OUTPUT_DIR"; exit 1; }

# Copy the content of the template directory to the output directory
cp -r "$TEMPLATE_DIR"/* "$OUTPUT_DIR/" || { echo "Failed to copy template files to $OUTPUT_DIR"; exit 1; }

# now parse the ff file to extract timestamp and latlon of the fire from init
#expecting lines like :
#loadData[data.nc;2024-09-16T12:22:50Z]
#startFire[lonlat=(-8.175970, 39.972564,0);t=0.0]

FF_FILE="$FF_UNCOUPLED_CASE/log.ff"
TIMESTAMP=$(sed -nE 's/.*loadData\[.*;(.*)\].*/\1/p' "$FF_FILE")
LON_START=$(sed -nE 's/.*startFire\[lonlat=\(([-0-9.]+), *([-0-9.]+), *([-0-9.]+)\);t=[0-9.]+\].*/\1/p' "$FF_FILE")
LAT_START=$(sed -nE 's/.*startFire\[lonlat=\(([-0-9.]+), *([-0-9.]+), *([-0-9.]+)\);t=[0-9.]+\].*/\2/p' "$FF_FILE")
ALT_START=$(sed -nE 's/.*startFire\[lonlat=\(([-0-9.]+), *([-0-9.]+), *([-0-9.]+)\);t=[0-9.]+\].*/\3/p' "$FF_FILE")
T_START=$(sed -nE 's/.*startFire\[lonlat=\([-0-9., ]+\);t=([0-9.]+)\].*/\1/p' "$FF_FILE")

echo "Timestamp: $TIMESTAMP"
echo "Fire start: LAT_START = $LAT_START, LON_START = $LON_START, ALT_START = $ALT_START"
# Check if TIMESTAMP, LAT_START, LON_START, and ALT_START were successfully extracted
if [ -z "$TIMESTAMP" ] || [ -z "$LAT_START" ] || [ -z "$LON_START" ] || [ -z "$ALT_START" ]; then
    echo "Error: Failed to extract required values (timestamp or latitude/longitude) from $FF_FILE"
    exit 1
fi

# copy the data tile
# Extract the data file path
DATA_TILE_FILE=$(sed -nE 's/.*loadData\[(.*);.*\].*/\1/p' "$FF_FILE")

# Copy the data file to "$OUTPUT_DIR/ForeFire/"
echo "copying: $DATA_TILE_FILE to $OUTPUT_DIR/ForeFire/"
cp "$DATA_TILE_FILE" "$OUTPUT_DIR/ForeFire/" || { echo "Failed to copy $DATA_FILE"; exit 1; }
cp "$FF_UNCOUPLED_CASE/log.ff" "$OUTPUT_DIR/ForeFire/"

# Extract date directory from the timestamp (YYYYMMDD format)
if date --version >/dev/null 2>&1; then
    DATEDIR_YYYMMMDD=$(date -d "$TIMESTAMP" +%Y%m%d)
else
    DATEDIR_YYYMMMDD=$(date -j -f "%Y-%m-%dT%H:%M:%SZ" "$TIMESTAMP" +%Y%m%d)
fi

BC_DIR="$BC_FILES/$DATEDIR_YYYMMMDD"
# If the BC directory does not exist or is empty, download the BC files from the web
if [ ! -d "$BC_DIR" ] || [ -z "$(ls -A "$BC_DIR")" ]; then
    mkdir -p "$BC_DIR" || { echo "Failed to create BC directory: $BC_DIR"; exit 1; }
    for STEP in 00 03 06 09 12 15 18 21 24 27 30 33 36 39 42 45 48; do
        echo "Downloading cep.FC00Z.$STEP"
        curl -o "$BC_DIR/cep.FC00Z.$STEP" "https://forefire.univ-corse.fr/saphir/MARS/$DATEDIR_YYYMMMDD/cep.FC00Z.$STEP" || { echo "Failed to download step $STEP"; exit 1; }
    done
fi

echo "Date directory: $DATEDIR_YYYMMMDD found in $BC_DIR"

# Link forecast files around ignition time
IGNITION_HOUR=$(date -u -j -f "%Y-%m-%dT%H:%M:%SZ" "$TIMESTAMP" +%H 2>/dev/null || date -d "$TIMESTAMP" +%H)
IGNITION_HOUR=$((10#$IGNITION_HOUR))
BASE_STEP=$(( (IGNITION_HOUR / 3) * 3 ))
printf -v BASE_STEP_STR "%02d" $BASE_STEP





ln -sf "$BC_DIR/cep.FC00Z.$BASE_STEP_STR" "$OUTPUT_DIR/cep.FC00Z.P.00"

for OFFSET in -3 3 6 9 12 15; do
    STEP=$(( BASE_STEP + OFFSET ))
    [ $STEP -lt 0 ] && continue
    printf -v STEP_STR "%02d" $STEP
    if [ $OFFSET -lt 0 ]; then
        SIGN="M"
        LABEL=$(( -1 * OFFSET ))
    else
        SIGN="P"
        LABEL=$OFFSET
    fi
    printf -v LABEL_STR "%02d" $LABEL
    ln -sf "$BC_DIR/cep.FC00Z.$STEP_STR" "$OUTPUT_DIR/cep.FC00Z.${SIGN}.${LABEL_STR}"
    echo "linking directory: cep.FC00Z.$STEP_STR to cep.FC00Z.${SIGN}.${LABEL_STR}"
done



# Update report placeholders with the correct ignition values
sed "${SED_INPLACE[@]}" -E "s/IGNITIONTIME/${TIMESTAMP}/" "$OUTPUT_DIR/report/report.tex"
sed "${SED_INPLACE[@]}" -E "s/LATIGNITION/${LAT_START}/" "$OUTPUT_DIR/report/report.tex"
sed "${SED_INPLACE[@]}" -E "s/LONIGNITION/${LON_START}/" "$OUTPUT_DIR/report/report.tex"

# Compute ignition epoch in a cross-platform way
if date --version >/dev/null 2>&1; then
    IGNITION_EPOCH=$(date -u -d "$TIMESTAMP" +%s)
else
    IGNITION_EPOCH=$(date -u -j -f "%Y-%m-%dT%H:%M:%SZ" "$TIMESTAMP" +%s)
fi

# Update time markers for figures from 1HAFTERSTARTFIRE to 12HAFTERSTARTFIRE
for HOUR in {1..12}; do
    NEW_EPOCH=$(( IGNITION_EPOCH + HOUR * 3600 ))
    if date --version >/dev/null 2>&1; then
        NEW_TIME=$(date -u -d "@$NEW_EPOCH" "+%Y-%m-%dT%H:%M:%SZ")
    else
        NEW_TIME=$(date -u -r $NEW_EPOCH "+%Y-%m-%dT%H:%M:%SZ")
    fi
    sed "${SED_INPLACE[@]}" -E "s/${HOUR}HAFTERSTARTFIRE/${NEW_TIME}/g" "$OUTPUT_DIR/report/report.tex" || { echo "Failed to update ${HOUR}HAFTERSTARTFIRE in report.tex"; exit 1; }
done


PGD_LARGEST_NAMELIST_FILE="$OUTPUT_DIR/PRE_PGD1.nam_LARGESTm"

# Update XLAT0 and XLON0 in NAM_CONF_PROJ using LAT_START and LON_START
sed "${SED_INPLACE[@]}" -E "s/(XLAT0 *=)[^,]*/\1$LAT_START/" "$PGD_LARGEST_NAMELIST_FILE"
sed "${SED_INPLACE[@]}" -E "s/(XLON0 *=)[^,]*/\1$LON_START/" "$PGD_LARGEST_NAMELIST_FILE"

# Update XLATCEN and XLONCEN in NAM_CONF_PROJ_GRID using LAT_START and LON_START
sed "${SED_INPLACE[@]}" -E "s/(XLATCEN *=)[^,]*/\1$LAT_START/" "$PGD_LARGEST_NAMELIST_FILE"
sed "${SED_INPLACE[@]}" -E "s/(XLONCEN *=)[^,]*/\1$LON_START/" "$PGD_LARGEST_NAMELIST_FILE"
echo "Updating PGD namelist file: $PGD_LARGEST_NAMELIST_FILE with LAT_START = $LAT_START and LON_START = $LON_START"


cd "$OUTPUT_DIR"
# Convert the full timestamp to epoch seconds.
timestamp_secs=$(date -u -j -f "%Y-%m-%dT%H:%M:%SZ" "$TIMESTAMP" +%s)
# Extract the date component (YYYY-MM-DD) from TIMESTAMP.
DATE_ONLY=$(date -u -j -f "%Y-%m-%dT%H:%M:%SZ" "$TIMESTAMP" "+%Y-%m-%d")
# Create a midnight timestamp.
midnight="${DATE_ONLY}T00:00:00Z"
midnight_secs=$(date -u -j -f "%Y-%m-%dT%H:%M:%SZ" "$midnight" +%s)
# Calculate the difference.
SECONDS_SINCE_MIDNIGHT=$(( timestamp_secs - midnight_secs ))
echo "$SECONDS_SINCE_MIDNIGHT"
ROUNDED_HOUR_SECONDS=$(( SECONDS_SINCE_MIDNIGHT / 3600 * 3600 + 3600))
# now i just have to do the init.ff
echo "FireDomain[pgdNcFile=PGD_DFIREmA.nc;ISOdate=${TIMESTAMP}]" > ForeFire/Init.ff
echo "startFire[lonlat=($LON_START, $LAT_START,0);t=$SECONDS_SINCE_MIDNIGHT]" >> ForeFire/Init.ff
echo "include[ForeFire/hourlyPlots.ff]@t=$ROUNDED_HOUR_SECONDS" >> ForeFire/Init.ff
#echo "listenHTTP[host=127.0.0.1:8080]" >> ForeFire/Init.ff
echo "Created Init file as :"
cat ForeFire/Init.ff






. run_build_pgd_and_nest && echo "PGD OK completed"

#linking the PGD to infer domain
ln -s PGD_DFIREmA.nc ForeFire/PGD_DFIREmA.nc

. run_ecmwf2mnh_xyz
. run_run_mnh_M1
. run_spawn_real
. run_run_mnh_fire
$PYTHONEXE BuildReport.py ; pdflatex -interaction=nonstopmode report/report.tex
#. run_VTK_for_paraview
#. run_plots_report