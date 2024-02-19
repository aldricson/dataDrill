#!/bin/bash

# Directory to check
directory="/home/dataDrill/"

# Get the total size of the partition where the directory is located
# Using POSIX compliant options
totalSize=$(df -k "$directory" | awk 'NR==2 {print $2}')

# Get the total size of the directory in kilobytes
dirSize=$(du -sk "$directory" | cut -f1)

# Check for division by zero
if [ "$totalSize" -eq 0 ]; then
    echo "Error: Unable to determine the total size of the partition."
    exit 1
fi

# Calculate the percentage usage
percentage=$(awk -v dirSize="$dirSize" -v totalSize="$totalSize" 'BEGIN {printf "%.2f", (dirSize/totalSize)*100}')

echo "$percentage"
