import subprocess
import os
import pathlib
import csv
from collections import defaultdict

FILEPATH = pathlib.Path(__file__).resolve().parent
BASE_PATH = os.path.join(FILEPATH)
SABLE_PATH = os.path.join(BASE_PATH, "..")

def check_file_matches_parent_dir(filepath):
    """
    Check if a file's name (without suffix) matches its parent directory name.
    
    Args:
        filepath (str): Full path to the file
        
    Returns:
        bool: True if file name (without suffix) matches parent directory name
        
    Example:
        >>> path = '/local/scratch/a/das160/SABLE/Suitesparse/GD96_a/GD96_a.mtx'
        >>> check_file_matches_parent_dir(path)
        True
    """
    # Get the file name without extension
    file_name = os.path.splitext(os.path.basename(filepath))[0]
    
    # Get the parent directory name
    parent_dir = os.path.basename(os.path.dirname(filepath))
    
    return file_name == parent_dir

if __name__ == "__main__":
    THREADS = [1,2,4,8]
    mtx_dir = os.path.join(SABLE_PATH, "Suitesparse")
    benchlist = ["bench_inspector", "bench_executor"]
    ops = ["SPMV"]
    # Adjust if hyperthreading is enabled
    cores = [i for i in range(0, 8)]
    for benchfile in benchlist:
        for threads in THREADS:
            str_cores = ",".join(map(str, cores[:threads]))
            for op in ops:
                with open(benchfile+"_"+str(threads)+"thrds_" + op + ".csv", "w") as f:
                    f.write("Matrix,Time(ns)\n")
                    for file_path in pathlib.Path(mtx_dir).rglob("*"):
                        if file_path.is_file() and file_path.suffix == ".mtx" and check_file_matches_parent_dir(file_path):
                            fname = pathlib.Path(file_path).resolve().stem
                            print(f"Benchmarking {fname} with {threads} threads")
                            f.write(fname)
                            try:
                                output = subprocess.check_output(["taskset", "-a", "-c", str_cores, f"{BASE_PATH}/build/DDT", "-m", file_path, "-n", op, "-s", "CSR", "--"+benchfile, "-t", str(threads)])
                            except subprocess.CalledProcessError as err:
                                print(fname + " failed with " + str(err))
                                continue
                            psc_times = output.decode("utf-8").split("\n")[0]
                            print(psc_times)
                            # Write the output to the file
                            # for time in psc_times:
                                # f.write(f",{time}")
                            # f.write("\n")
                            f.write(f",{psc_times}\n")
                            f.flush()

        # Merge the CSV files
        merged_data = defaultdict(list)
        matrix_set = set()

        # Read all thread-specific files
        for threads in THREADS:
            for op in ops:
                filename = f"{benchfile}_{threads}thrds_{op}.csv"
                with open(filename, "r") as f:
                    reader = csv.reader(f)
                    next(reader)  # Skip header
                    for row in reader:
                        if row:
                            matrix = row[0]
                            matrix_set.add(matrix)
                            time_val = row[1] if len(row) > 1 else ""
                            merged_data[matrix].append(time_val)

        # Write merged output
        with open(f"{benchfile}_{op}_merged.csv", "w") as merged_file:
            merged_file.write("Matrix")
            for threads in THREADS:
                for op in ops:
                    merged_file.write(f",{threads} Threads")
            merged_file.write("\n")

            for matrix in sorted(matrix_set):
                merged_file.write(matrix)
                times = merged_data.get(matrix, [])
                for time in times:
                    merged_file.write(f",{time}")
                # Fill in any missing times (if some thread configs failed)
                if len(times) < len(THREADS) * len(ops):
                    merged_file.write("," * (len(THREADS) * len(ops) - len(times)))
                merged_file.write("\n")

                