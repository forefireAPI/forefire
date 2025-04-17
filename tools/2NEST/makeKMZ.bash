#!/bin/bash

# Check if an argument is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <parameter_suffix>"
    exit 1
fi

suffix="$1"

# Define an array of parameters and their corresponding image file names
# Format: "parameterName:imageFile"
entries=(
    "plumeTopHeight:plumeTopHeight.png"
    "plumeBottomHeight:plumeBottomHeight.png"
    "smokeAtGround:smokeAtGround.png"
    "tke:tke.png"
    "wind:wind.png"
    "speed:ros.png"
)

# Check if the images directory exists
OUTPUT_DIR="images"
if [ ! -d "$OUTPUT_DIR" ]; then
    echo "Error: Directory $OUTPUT_DIR not found."
    exit 1
fi

# Iterate over each entry to create the KMZ files
for entry in "${entries[@]}"; do
    IFS=":" read -r param imageFile <<< "$entry"
    
    # Construct file names
    KML_FILE="${param}.kml"
    OUTPUT_FILE="${OUTPUT_DIR}/${param}_${suffix}.kmz"
    IMAGE_PATH="${imageFile}"
    
    # Check existence of the KML file
    if [ ! -f "$KML_FILE" ]; then
        echo "Error: KML file $KML_FILE not found. Skipping $param."
        continue
    fi
    
    # Check existence of the image file
    if [ ! -f "$IMAGE_PATH" ]; then
        echo "Error: Image file $IMAGE_PATH not found. Skipping $param."
        continue
    fi
    
    # Create the KMZ file by zipping the KML file and the corresponding image file
    zip -j "$OUTPUT_FILE" "$KML_FILE" "$IMAGE_PATH" > /dev/null
    if [ $? -eq 0 ]; then
        echo "KMZ file created successfully: $OUTPUT_FILE"
    else
        echo "Error creating KMZ file for $param."
    fi

done
