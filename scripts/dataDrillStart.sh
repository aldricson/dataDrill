#!/bin/bash

#export LD_LIBRARY_PATH=/home/dataDrill/libraries

# Check if a screen session with the name "dataDrill" already exists
existing_session=$(screen -ls | grep "dataDrill")

# If it doesn't exist, create it
if [ -z "$existing_session" ]; then
  screen -S "dataDrill" -d -m ./dataDrill
  echo "Started program in new screen session."
else
  echo "A screen session with this program already exists."
fi
