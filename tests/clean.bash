#!/bin/bash

for cleandir in mnh/ideal mnh/real_nested python runANN runff; do

    if  [ -d "$cleandir" ]; then
        echo "cleaning $cleandir..." 
        (cd "$cleandir" && ./clean.bash)
    else
        echo "Directory $cleandir not found; skipping."
    fi
done