#!/bin/bash

# Process name
processName="dataDrill"

# Linux version
linuxVersion=$(uname -r)

# System uptime
uptimeSeconds=$(awk '{print int($1)}' /proc/uptime)
uptimeFormatted=$(printf "%d years, %d months, %d days, %d hours, %d minutes\n" \
    $((uptimeSeconds/31536000)) \
    $((uptimeSeconds%31536000/2592000)) \
    $((uptimeSeconds%2592000/86400)) \
    $((uptimeSeconds%86400/3600)) \
    $((uptimeSeconds%3600/60)))


# dataDrill RAM consumption (in Mo)
dataDrillRAM=$( ps -o rss,comm | grep -w "$processName" | sed 's/m.*//' | awk '{print $1 " Mo"}')


# Total and Free HDD size
totalHDD=$(df -h / | awk 'NR==2 {print $2}')
freeHDD=$(df -h / | awk 'NR==2 {print $4}')

# Available RAM
availableRAM=$(free -m | awk '/Mem:/ {print $7 " Mo"}')

# Most RAM consuming app
mostRAMApp=$(ps -o comm,rss | sort -k2 -n -r | head -n 1 | awk '{print $1}')

# Most CPU consuming app
mostCPUApp=$(ps -o comm,time | sort -k2 -n -r | head -n 1 | awk '{print $1}')

# Output the information
echo "Linux version            : $linuxVersion"
echo "System uptime            : $uptimeFormatted"
echo "dataDrill RAM consumption: $dataDrillRAM"
echo "Total HDD size           : $totalHDD"
echo "Free HDD size            : $freeHDD"
echo "Available RAM            : $availableRAM"
echo "Most RAM consuming app   : $mostRAMApp"
echo "Most CPU consuming app   : $mostCPUApp"
