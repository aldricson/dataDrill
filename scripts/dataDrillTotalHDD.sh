#!/bin/bash

# Specify the target partition or mount point, e.g., "/" for the root filesystem
partition="/"

# Use the `df` command to get disk usage and parse the output for the specified partition
# The awk command extracts the numeric percentage of used space for the first match only
usedSpace=$(df -h | awk -v partition="$partition" '$6 == partition {gsub(/%/, "", $5); print $5; exit}')

echo "$usedSpace"
