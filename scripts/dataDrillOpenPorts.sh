#!/bin/bash

# Define the ports to check
modbus_port=502
command_port=8222
debug_port=8223

# Check Modbus port (502)
modbus_scan=$(nmap 127.0.0.1 -p $modbus_port | grep "$modbus_port" | awk '{print $2}')
if [ "$modbus_scan" == "open" ]; then
    echo "Modbus port open"
else
    echo "Modbus port not open"
fi

# Check command port (8222)
command_scan=$(nmap 127.0.0.1 -p $command_port | grep "$command_port" | awk '{print $2}')
if [ "$command_scan" == "open" ]; then
    echo "Command port open"
else
    echo "Command port not open"
fi

# Check command port (8223)
command_scan=$(nmap 127.0.0.1 -p $debug_port | grep "$command_port" | awk '{print $2}')
if [ "$command_scan" == "open" ]; then
    echo "Debug port open"
else
    echo "Debug port not open"
fi

