#!/bin/bash

# Find the session ID of the screen session with the name "dataDrill"
session_id=$(screen -ls | grep "dataDrill" | cut -d '.' -f 1)

# If it exists, reattach to it
if [ -n "$session_id" ]; then
  screen -r $session_id
else
  echo "No screen session with this program found."
fi
