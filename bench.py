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
                    subprocess.call([f"{BASE_PATH}/partially-strided-codelet/build/DDT", "-m", os.path.join(VBR_PATH, filename), "-n", "SPMV", "-s", "CSR", "--"+benchfile, "-t", str(threads)], stdout=subprocess.PIPE)
                    for i in range(5):
                        print(f"{i+1}th iteration")
                        output = subprocess.run([f"{BASE_PATH}/partially-strided-codelet/build/DDT", "-m", os.path.join(VBR_PATH, filename), "-n", "SPMV", "-s", "CSR", "--"+benchfile, "-t", str(threads)], capture_output=True)
                        f.write(","+output.stdout.decode("utf-8")[:-1])
                    f.write("\n")
