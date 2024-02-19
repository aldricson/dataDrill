#!/bin/bash

while true; do
    echo "-----------------------------------"
    echo " Master Control Script"
    echo "-----------------------------------"
    echo "1. Start program"
    echo "2. Reattach to program session"
    echo "3. Kill program"
	echo "4. Restart Program"
    echo "5. List open TCP ports by program"
	echo "6. Program Status"
    echo "7. Exit"
    echo "-----------------------------------"
    echo -n "Enter your choice: "
    read choice

    case $choice in
        1)
            echo "Starting program..."
            ./dataDrillStart.sh
            ;;
        2)
            echo "Reattaching to program session..."
            ./dataDrillReAttach.sh
            ;;
        3)
            echo "Killing program..."
            ./dataDrillKill.sh
            ;;
		4)
            echo "Restarting program..."
            ./dataDrillRestart.sh
            ;;
        5)
            echo "Listing open TCP ports..."
            ./dataDrillOpenPorts.sh
            ;;
		6)
            echo "Program Status"
            ./dataDrillStatus.sh
            ;;
        7)
            echo "Exiting."
            exit 0
            ;;
        *)
            echo "Invalid choice, please try again."
            ;;
    esac

    # Pause before showing menu again
    echo "Press any key to continue..."
    read -n1 -s
    clear
done
