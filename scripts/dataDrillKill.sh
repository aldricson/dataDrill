#!/bin/bash

# Find the session ID of the screen session with the name "dataDrill"
session_id=$(screen -ls | grep "dataDrill" | cut -d '.' -f 1)

# If it exists, detach it
if [ -n "$session_id" ]; then
  screen -X -S $session_id quit
  echo "Detached and terminated screen session."
else
  echo "No screen session with this program found."
fi

# Find the process ID of the program and kill it
# Using -x to match only the exact name
program_pid=$(pgrep -x "dataDrill")
if [ -n "$program_pid" ]; then
  kill -9 $program_pid
  echo "Killed program."
else
  echo "Program is not running."
fi
