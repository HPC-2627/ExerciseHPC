#!/bin/bash

# List of all exercise directories
DIRS=(
    "Exercise1:Communication"
    "Exercise2:Communication"
    "Exercise3:PingPong"
    "Exercise4:OpenClosedRing"
    "Exercise5:Reduce"
    "Exercise6:AllReduce"
    "Exercise7:PingPongAd"
    "Exercise8:BroadcastTreeStructured"
)

echo "================================================="
echo "    Submitting all exercises to PBS queue...     "
echo "================================================="

for dir in "${DIRS[@]}"; do
    if [ -d "$dir" ]; then
        echo "--> Entering directory: $dir"
        cd "$dir" || continue
        
        if [ -f "run.sh" ]; then
            echo "   Submitting $dir/run.sh..."
            qsub run.sh
        else
            echo "   [WARNING] run.sh not found in $dir"
        fi
        
        # Return to root directory
        cd ..
    else
        echo "   [ERROR] Directory $dir not found"
    fi
    echo "-------------------------------------------------"
done

echo "Submission completed! Check status using 'qstat -u $USER'."