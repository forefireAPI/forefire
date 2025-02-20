#!/bin/bash

for cleandir in mnh_ideal mnh_real_nested python runANN runff; do

    if  [ -d "$cleandir" ]; then
        echo "cleaning $cleandir..." 
        cd "$cleandir" && ./clean.bash
        cd ..
    else
        echo "Directory $cleandir not found; skipping."
    fi
done