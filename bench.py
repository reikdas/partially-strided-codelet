import subprocess
import os
import pathlib

FILEPATH = pathlib.Path(__file__).resolve().parent
BASE_PATH = os.path.join(FILEPATH, "..")

if __name__ == "__main__":
    benchlist = ["bench_inspector", "bench_executor"]
    for benchfile in benchlist:
        for threads in [1, 2, 4, 8, 16]:
            with open(benchfile+"_"+str(threads)+"thrds.csv", "w") as f:
                VBR_PATH = f"{BASE_PATH}/Generated_MMarket"
                for filename in os.listdir(VBR_PATH):
                    print(f"Benchmarking {filename} with {threads} threads")
                    assert(filename.endswith(".mtx"))
                    f.write(f"{filename[:-4]}")
                    try:
                        output = subprocess.run([f"{BASE_PATH}/partially-strided-codelet/build/demo/spmm_demo", "-m", os.path.join(VBR_PATH, filename), "-n", "SPMM", "-s", "CSR", "--"+benchfile, "-t", str(threads)], capture_output=True, check=True)
                    except subprocess.CalledProcessError as err:
                        print(filename + " failed with " + str(err))
                        continue
                    psc_times = output.stdout.decode("utf-8").split("\n")[:-1]
                    # Write the output to the file
                    for time in psc_times:
                        f.write(f",{time}")
                    f.write("\n")
                    f.flush()
