#!/bin/bash

# Read the first line from /proc/stat
read -r cpu line < /proc/stat

# Split the line into its components
set -- $line

# Calculate total CPU time
totalTime=$(( $1 + $2 + $3 + $4 + $5 + $6 + $7 ))

# Calculate idle time
idleTime=$4

# Calculate the total and idle times in the next sample
sleep 1
read -r cpu line < /proc/stat
set -- $line
totalTimeNew=$(( $1 + $2 + $3 + $4 + $5 + $6 + $7 ))
idleTimeNew=$4

# Calculate the change in total and idle times
totalDelta=$(( totalTimeNew - totalTime ))
idleDelta=$(( idleTimeNew - idleTime ))

# Calculate the CPU usage as a percentage using bc for floating point precision
cpuUsage=$(echo "(100 * ($totalDelta - $idleDelta) / $totalDelta)" | bc -l | awk '{printf "%.2f", $0}')

echo "$cpuUsage"
