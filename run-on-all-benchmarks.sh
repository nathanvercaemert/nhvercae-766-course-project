#!/bin/bash

# Get the path to the program
program_path="/root/766-volume/nhvercae/i-o-analysis/i-o-analysis"

# Find all .c files in the current directory and its subdirectories
find . -type f -name "*.c" | while read -r file; do
    # Check if the file exists
    if [ -e "$file" ]; then
        # Backup the original file
        cp "$file" "${file}.bak"

        # Comment out include statements
        sed -i 's/^\(\s*#include\)/\/\/\1/' "$file"

        # Execute the program on the file and capture the output
        output=$($program_path "$file")

        # Check if the output is non-empty
        if [ -n "$output" ]; then

            # Print the file name
            echo "Processing file: $file"

            # Print the program's output
            echo "$output"
        fi

        # Restore the original file
        mv "${file}.bak" "$file"
    fi
done
