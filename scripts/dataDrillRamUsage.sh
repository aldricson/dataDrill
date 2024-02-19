#!/bin/bash

# Process name
processName="dataDrill"

# Get the total memory used by all instances of the process in kilobytes
totalMemProcess=$(ps -o rss,comm | grep "$processName" | awk '{sum+=$1} END {print sum}')

# Get the total system memory in kilobytes
totalSystemMem=$(free | awk '/Mem:/ {print $2}')

# Check if total system memory is available
if [ -z "$totalSystemMem" ] || [ "$totalSystemMem" -eq 0 ]; then
    echo "Error: Unable to determine the total system memory."
    exit 1
fi

# Calculate the memory usage percentage
memUsagePercent=$(awk -v totalMemProcess="$totalMemProcess" -v totalSystemMem="$totalSystemMem" 'BEGIN {printf "%.2f", (totalMemProcess/totalSystemMem)*100}')

echo "$memUsagePercent"
