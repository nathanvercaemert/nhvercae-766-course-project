#!/bin/bash

# Check if both arguments are provided
if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Usage: $0 <program_path> <directory>"
    exit 1
fi

# Get the path to the program from the first argument
program_path="$1"

# Get the target directory from the second argument
target_directory="$2"

# Find all .c files in the target directory and its subdirectories
find "$target_directory" -type f -name "*.c" | while read -r file; do
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
