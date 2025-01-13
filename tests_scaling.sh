#!/bin/bash

# Output file
output_file="tests_scaling.txt"

# Clear the output file if it exists
> "$output_file"

# Loop through the values of l
for n in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16; do
  echo "Running with nproc = $n" | tee -a "$output_file"

  # Run the command using /usr/bin/time -v and append both stdout and stderr to the output file
  /usr/bin/time -v -a -o "$output_file" mpirun -n $n ./run-epanet3 -h 24 -a 2 -l 9 >> "$output_file" 2>&1

  echo "-------------------------" >> "$output_file"
done

echo "All runs completed. Output saved to $output_file"