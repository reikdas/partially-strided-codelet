import subprocess
import os

if __name__ == "__main__":
    with open("benchmarks.txt", "w") as f:
        VBR_PATH = "/home/reikdas/VBR-SpMV/Generated_Matrix"
        for filename in os.listdir(VBR_PATH):
            assert(filename.endswith(".mtx"))
            print(filename)
            subprocess.call(["/home/reikdas/partially-strided-codelet/build/DDT", "-m", os.path.join(VBR_PATH, filename), "-n", "SPMV", "-s", "CSR"], stdout=subprocess.PIPE)
            sum = 0
            for i in range(5):
                output = subprocess.run(["/home/reikdas/partially-strided-codelet/build/DDT", "-m", os.path.join(VBR_PATH, filename), "-n", "SPMV", "-s", "CSR"], capture_output=True)
                sum += float(output.stdout.decode("utf-8"))
            avg = sum/5
            f.write(f"{filename[:-4]}: {avg}ms\n")