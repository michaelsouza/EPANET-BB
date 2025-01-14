#!/bin/bash

# Output files
output_time_file="tests_level_time.txt"
output_memory_file="tests_level_memory.txt"

# Clear the output files if they exist
> "$output_time_file"
> "$output_memory_file"

# Function to get the system memory usage with a timestamp
get_memory_usage() {
  local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
  local memory_usage=$(free -h | awk '/^Mem:/{print $3}')
  echo "$timestamp $memory_usage"
}

# Function to monitor memory usage for a given PID
monitor_memory() {
  local pid=$1
  local l_value=$2

  echo "Memory usage for l = $l_value:" >> "$output_memory_file"
  
  # Continuously monitor until the process exits
  while kill -0 "$pid" 2>/dev/null; do
    get_memory_usage >> "$output_memory_file"
    sleep 1
  done
}

# Loop through the values of l
for l in {3..10}; do
  echo "Running with l = $l" | tee -a "$output_time_file"

  # Capture initial memory usage before starting mpirun
  echo "Initial memory usage for l = $l:" >> "$output_memory_file"
  get_memory_usage >> "$output_memory_file"

  # Start the mpirun command with time in the background and capture its PID
  /usr/bin/time -v -a -o "$output_time_file" mpirun -n 16 ./run-epanet3 -h 24 -a 2 -l "$l" >> "$output_time_file" 2>&1 &
  cmd_pid=$!

  # Start monitoring memory usage in the background
  monitor_memory "$cmd_pid" "$l" &

  # Wait for the mpirun command to finish
  wait "$cmd_pid"

  echo "-------------------------" | tee -a "$output_time_file"
done

echo "All runs completed. Output saved to $output_time_file and $output_memory_file"
