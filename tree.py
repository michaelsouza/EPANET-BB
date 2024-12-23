# tree.py

import subprocess

def run_tree_command(directory):
    """Runs the tree command on the specified directory and returns the output."""
    try:
        result = subprocess.run(['tree', directory], capture_output=True, text=True, check=True)
        return result.stdout
    except subprocess.CalledProcessError as e:
        return f"An error occurred while processing {directory}: {e}"

def main(directories):
    with open("tree.txt", "w") as file:
        for directory in directories:
            file.write(f"Directory structure for: {directory}\n")
            output = run_tree_command(directory)
            file.write(output)
            file.write("\n")  # Add a newline between directory outputs

if __name__ == "__main__":
    directories = ["include", "src", "run"]
    main(directories)

