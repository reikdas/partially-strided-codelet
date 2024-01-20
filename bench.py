import subprocess
import os

if __name__ == "__main__":
    benchlist = ["bench_inspector", "bench_executor"]
    for benchfile in benchlist:
        for threads in [1, 2, 4, 8, 16]:
            with open(benchfile+"_"+str(threads)+"thrds.txt", "w") as f:
                VBR_PATH = "/home/reikdas/VBR-SpMV/Generated_Matrix"
                for filename in os.listdir(VBR_PATH):
                    assert(filename.endswith(".mtx"))
                    f.write(f"{filename[:-4]}")
                    subprocess.call(["/home/reikdas/partially-strided-codelet/build/DDT", "-m", os.path.join(VBR_PATH, filename), "-n", "SPMV", "-s", "CSR", "--"+benchfile, "-t", str(threads)], stdout=subprocess.PIPE)
                    for i in range(5):
                        output = subprocess.run(["/home/reikdas/partially-strided-codelet/build/DDT", "-m", os.path.join(VBR_PATH, filename), "-n", "SPMV", "-s", "CSR", "--"+benchfile, "-t", str(threads)], capture_output=True)
                        f.write(","+output.stdout.decode("utf-8")[:-1])
                    f.write("\n")