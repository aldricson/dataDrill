#!/bin/bash

# Define the path to the thermal zone file.
# Note: The thermal zone index (0, 1, 2, ...) might vary depending on your hardware.
# Check the /sys/class/thermal/ directory for the correct thermal_zone index.
temp_file="/sys/class/thermal/thermal_zone0/temp"

# Check if the temperature file exists
if [ -f "$temp_file" ]; then
    # Read the temperature (usually in millidegrees Celsius)
    temp=$(cat "$temp_file")

    # Convert the temperature to degrees Celsius
    temp=$((temp / 1000))

    # Output the CPU temperature
    echo "CPU Temperature: $temp°C"
else
    echo "CPU temperature information not available."
fi
