#!/bin/bash

# Get memory usage data using 'free' command
memData=$(free)

# Extract the total and used memory in kilobytes
totalMem=$(echo "$memData" | awk '/Mem:/ {print $2}')
usedMem=$(echo "$memData" | awk '/Mem:/ {print $3}')

# Check if total memory value is available
if [ -z "$totalMem" ] || [ "$totalMem" -eq 0 ]; then
    echo "Error: Unable to determine the total system memory."
    exit 1
fi

# Calculate the RAM usage percentage
ramUsagePercent=$(awk -v total="$totalMem" -v used="$usedMem" 'BEGIN {printf "%.2f", (used/total)*100}')

echo "$ramUsagePercent"
