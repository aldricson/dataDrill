#!/bin/bash
session_id=$(screen -ls | grep "dataDrill" | cut -d '.' -f 1)
program_pid=$(pgrep -x "dataDrill")  # or use -f with more specific string

if [ -n "$session_id" ]; then
  echo "Program is running in screen session: $session_id"
else
  echo "Program is not running in any screen session."
fi

if [ -n "$program_pid" ]; then
  echo "Program is running with PID: $program_pid"
else
  echo "Program is not running."
fi

