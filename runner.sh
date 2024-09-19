#!/bin/bash

make clean
make all

# Path to the script you want to run
SCRIPT_TO_RUN="./bank -f input-1.txt"

# Number of times to run the script
COUNT=1 # changeg to 5000 to check for deadlocks

# Loop to run the script 5000 times
for ((i=1; i<=COUNT; i++))
do
  echo "Running iteration $i"
  $SCRIPT_TO_RUN
done
echo "finished"