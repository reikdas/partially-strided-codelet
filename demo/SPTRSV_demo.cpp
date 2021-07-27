//
// Created by kazem on 7/16/21.
//

#include <iostream>
#include <sparse_io.h>
#include <sparse_utilities.h>

#include "SPTRSV_demo_utils.h"

#ifdef METIS
#include "metis_interface.h"
#endif


using namespace sparse_avx;
int main(int argc, char *argv[]) {
    auto config = DDT::parseInput(argc, argv);
    int num_threads = config.nThread;
    int coarsening_p = config.coarsening;
    bool bpack = config.bin_packing;
    int initial_cut = config.coarsening;
    int tuning = config.tuning_mode;
    sym_lib::CSC *A;
    A = sym_lib::read_mtx(config.matrixPath);
    auto *sol = new double[A->n]();
    int *perm;
    std::fill_n(sol, A->n, 1);
    sym_lib::CSC *A_full = NULLPNTR;

#ifdef METIS
    //We only reorder L since dependency matters more in l-solve.
    sym_lib::CSC *L1_ord;
    if (A->stype < 0) {
        A_full = sym_lib::make_full(A);
        sym_lib::metis_perm_general(A_full, perm);
        sym_lib::CSC *Lt = transpose_symmetric(A, perm);
        L1_ord = transpose_symmetric(Lt, NULLPNTR);
        delete Lt;
        delete A_full;
        delete[] perm;
    } else {
        L1_ord = A;
    }
#endif
    sym_lib::CSR *L1_ord_csr = sym_lib::csc_to_csr(L1_ord);

    auto *sps = new SpTRSVSerial(L1_ord_csr, L1_ord, NULLPNTR, "Baseline");
    auto sptrsv_baseline = sps->evaluate();
    double *sol_sptrsv = sps->solution();

    auto *spsls = new SptrsvLevelSet(L1_ord_csr, L1_ord, sol_sptrsv,
                                     "Parallel Levelset");
    auto sptrsv_ls = spsls->evaluate();

    auto *spslsnv = new SptrsvLevelSetNovec(L1_ord_csr, L1_ord, sol_sptrsv,
                                            "Parallel Levelset NoVec");
    auto sptrsv_ls_novec = spslsnv->evaluate();

    auto *spsp = new SpTRSVParallel(L1_ord_csr, L1_ord, sol_sptrsv,
                                    "Parallel "
                                    "LBC",
                                    num_threads, coarsening_p, initial_cut, bpack);
    auto sptrsv_par = spsp->evaluate();

    auto *spspv2 =
            new SpTRSVParallelVec2(L1_ord_csr, L1_ord, sol_sptrsv,
                                   "Parallel Vec2"
                                   "LBC",
                                   num_threads, coarsening_p, initial_cut, bpack);
    auto sptrsv_parv2 = spspv2->evaluate();

//#ifdef DDTT
    config.nThread = 1;
 auto *ddtsptrsvst = new SpTRSVDDT(L1_ord_csr, L1_ord, sol_sptrsv, config,
                                 "SpTRSV DDT Serial", 1, coarsening_p,
                                 initial_cut);
 auto ddt_execst =  ddtsptrsvst->evaluate();
 auto ddt_analysisst = ddtsptrsvst->get_analysis_bw();

    config.nThread = num_threads;

    auto *ddtsptrsvmt = new SpTRSVDDT(L1_ord_csr, L1_ord, sol_sptrsv, config,
                                    "SpTRSV DDT", num_threads, coarsening_p,
                                    initial_cut);
    auto ddt_execmt =  ddtsptrsvmt->evaluate();
    auto ddt_analysismt = ddtsptrsvmt->get_analysis_bw();

//#endif
    auto *sptrsv_vec1 =
            new SpTRSVSerialVec1(L1_ord_csr, L1_ord, NULLPNTR, "SpTRSV_Vec1");
    auto sptrsv_vec1_exec = sptrsv_vec1->evaluate();

    auto *sptrsv_vec2 =
            new SpTRSVSerialVec2(L1_ord_csr, L1_ord, NULLPNTR, "SpTRSV_Vec2");
    auto sptrsv_vec2_exec = sptrsv_vec2->evaluate();

#ifdef MKL
    auto *mkltrsvst =
            new SpTRSVMKL(1, L1_ord_csr, L1_ord, NULLPNTR, "SpTRSV_MKL_SERIAL");
    auto sptrsv_mkl_execst = mkltrsvst->evaluate();

    auto *mkltrsvmt =
            new SpTRSVMKL(config.nThread, L1_ord_csr, L1_ord, NULLPNTR, "SpTRSV_MKL_SERIAL");
    auto sptrsv_mkl_execmt = mkltrsvmt->evaluate();
#endif

    if (config.header) {
        std::cout << "Matrix,Threads,Coarsening,Bin-packing,";
        std::cout << "Dimension,NNZ,";
        std::cout << "SpTRSV Base,SpTRSV Vec1, SpTRSV Vec2, SpTRSV LS Vec, "
                     "SpTRSV LS "
                     "NOVec, SpTRSV Parallel,SpTRSV "
                     "Vec2 Parallel,";
#ifdef MKL
        std::cout << "SpTRSV MKL Serial Executor,";
        std::cout << "SpTRSV MKL Parallel Executor,";
#endif
        std::cout << "SpTRSV DDT Serial Executor, SpTRSV DDT "
                     "Parallel Executor, Inspector_Time";
        std::cout << "\n";
    }

    std::cout << config.matrixPath << "," << config.nThread << ","
              << config.coarsening << ","<< config.bin_packing<<","
              << L1_ord->n<<","<<L1_ord->nnz<<","
               << sptrsv_baseline.elapsed_time << ","
              << sptrsv_vec1_exec.elapsed_time << ","
              << sptrsv_vec2_exec.elapsed_time << ",";
    std::cout << sptrsv_ls.elapsed_time << ",";
    std::cout << sptrsv_ls_novec.elapsed_time << ",";
    std::cout << sptrsv_par.elapsed_time << ",";
    std::cout << sptrsv_parv2.elapsed_time << ",";
#ifdef MKL
    std::cout << sptrsv_mkl_execst.elapsed_time << ",";
    std::cout << sptrsv_mkl_execmt.elapsed_time << ",";
#endif
//#ifdef DDTT
    std::cout << ddt_execst.elapsed_time << ",";
    std::cout << ddt_execmt.elapsed_time << ",";
//#endif
    std::cout << "\n";

    // delete A;
    delete L1_ord;
    delete L1_ord_csr;
    delete[] sol;

    delete sps;
    delete spsp;
    delete spsls;
    delete spslsnv;
    delete spspv2;
    //delete ddtsptrsv;
    delete sptrsv_vec1;
    delete sptrsv_vec2;

    // delete ddtsptrsv;

    return 0;
}
