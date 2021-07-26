//
// Created by kazem on 6/13/21.
//

#ifndef SPARSE_AVX512_DEMO_SPMVVEC_H
#define SPARSE_AVX512_DEMO_SPMVVEC_H

#include "FusionDemo.h"

#include "DDT.h"
#include "Executor.h"
#include "Input.h"
#include "Inspector.h"
#include "PatternMatching.h"

#include <iostream>
#ifdef MKL
#include <mkl.h>
#endif
#ifdef PAPI
#include "PAPIWrapper.h"
#include "Profiler.h"
#endif

namespace sparse_avx{

 ///// SPMV
 void spmv_csr(int n, const int *Ap, const int *Ai, const double *Ax,
               const double *x, double *y) {
  int i, j;
  for (i = 0; i < n; i++) {
   for (j = Ap[i]; j < Ap[i + 1]; j++) {
    y[i] += Ax[j] * x[Ai[j]];
   }
  }
 }

 void spmv_csr_parallel(int n, const int *Ap, const int *Ai, const double *Ax,
               const double *x, double *y, int nThreads) {
#pragma omp parallel for num_threads(nThreads)
  for (int i = 0; i < n; i++) {
   for (int j = Ap[i]; j < Ap[i + 1]; j++) {
    y[i] += Ax[j] * x[Ai[j]];
   }
  }
 }

 class SpMVSerial : public sym_lib::FusionDemo {
 protected:
  void setting_up() override {
   std::fill_n(x_,n_,0.0);
   std::fill_n(x_in_,n_,1.0);
  }

  sym_lib::timing_measurement fused_code() override {
   sym_lib::timing_measurement t1;
   t1.start_timer();
   spmv_csr(n_, L1_csr_->p, L1_csr_->i, L1_csr_->x, x_in_, x_);
   t1.measure_elapsed_time();
   //copy_vector(0,n_,x_in_,x_);
   return t1;
  }

 public:
  SpMVSerial(sym_lib::CSR *L, sym_lib::CSC *L_csc,
             double *correct_x,
             std::string name) :
    FusionDemo(L_csc->n, name) {
   L1_csr_ = L;
   L1_csc_ = L_csc;
   correct_x_ = correct_x;
#ifdef PROFILE
   this->pw_ = nullptr;
#endif
  };

  ~SpMVSerial() override = default;
 };

 class SpMVParallel : public SpMVSerial {
 protected:
  sym_lib::timing_measurement fused_code() override {
   sym_lib::timing_measurement t1;
   t1.start_timer();
   spmv_csr_parallel(n_, L1_csr_->p, L1_csr_->i, L1_csr_->x, x_in_, x_, num_threads_);
   t1.measure_elapsed_time();
   //copy_vector(0,n_,x_in_,x_);
   return t1;
  }

 public:
  SpMVParallel(sym_lib::CSR *L, sym_lib::CSC *L_csc,
           double *correct_x,
           std::string name) :
    SpMVSerial(L, L_csc, correct_x, name) {
   L1_csr_ = L;
   L1_csc_ = L_csc;
   correct_x_ = correct_x;
  };
 };


 void pruneIterations(DDT::MemoryTrace mt, int density) {
  auto t0 = std::chrono::steady_clock::now();
  for (int i = 0; i < mt.ips; i++) {
   int oneD = 0;
   while (mt.ip[i+1] - mt.ip[i] == 1) {
    ++oneD;
   }
//    mt.a.nt = i+oneD;
//    mt.a.t  = 0;
  }
  auto t1 = std::chrono::steady_clock::now();
 }

#ifdef MKL
 class SpMVMKL : public SpMVSerial {
     sparse_matrix_t m;
     matrix_descr d;
     MKL_INT* LLI;
     int num_threads;

     void build_set() override {
         if (this->LLI != nullptr)
             return;
         d.type = SPARSE_MATRIX_TYPE_GENERAL;
//         d.diag = SPARSE_DIAG_NON_UNIT;
//         d.mode = SPARSE_FILL_MODE_FULL;

         MKL_INT expected_calls = 5;

         LLI = new MKL_INT[this->L1_csr_->m+1]();
         for (int l = 0; l < this->L1_csr_->m+1; ++l) {
             LLI[l] = this->L1_csr_->p[l];
         }

         auto  stat = mkl_sparse_d_create_csr(&m, SPARSE_INDEX_BASE_ZERO, this->L1_csr_->m, this->L1_csr_->n,
                                 LLI, LLI+ 1, this->L1_csr_->i, this->L1_csr_->x);
         mkl_sparse_set_mv_hint(m, SPARSE_OPERATION_NON_TRANSPOSE, d, expected_calls);
         mkl_sparse_set_memory_hint(m, SPARSE_MEMORY_AGGRESSIVE);

         mkl_set_num_threads(num_threads);
         mkl_set_num_threads_local(num_threads);
     }
     sym_lib::timing_measurement fused_code() override {
         sym_lib::timing_measurement t1;
         t1.start_timer();
         mkl_sparse_d_mv(SPARSE_OPERATION_NON_TRANSPOSE, 1, m, this->d, this->x_in_, 1, this->x_);
         t1.measure_elapsed_time();
         return t1;
     }

 public:
     SpMVMKL(int nThreads, sym_lib::CSR *L, sym_lib::CSC *L_csc,
     double *correct_x,
     std::string name) :
     SpMVSerial(L, L_csc, correct_x, name), num_threads(nThreads), LLI(nullptr) {}

     ~SpMVMKL() override {
         delete[] LLI;
     }
 };
#endif

 class SpMVDDT : public SpMVSerial {
 protected:
  DDT::Config config;
  std::vector<DDT::Codelet*>* cl;
  DDT::GlobalObject d;
  sym_lib::timing_measurement analysis_breakdown;

  void build_set() override {
   // Allocate memory and generate global object
   if (this->cl == nullptr) {
       this->cl = new std::vector<DDT::Codelet *>[config.nThread];
       d = DDT::init(config);
       analysis_breakdown.start_timer();
       DDT::inspectSerialTrace(d, cl, config);
       analysis_breakdown.measure_elapsed_time();
   }
  }

  sym_lib::timing_measurement fused_code() override {
   sym_lib::timing_measurement t1;
   DDT::Args args; args.x = x_in_; args.y = x_;
   args.r = L1_csr_->m; args.Lp=L1_csr_->p; args.Li=L1_csr_->i;
   args.Lx = L1_csr_->x;
   t1.start_timer();
   // Execute codes
   DDT::executeCodelets(cl, config, args);
   t1.measure_elapsed_time();
   //copy_vector(0,n_,x_in_,x_);
   return t1;
  }

 public:
  SpMVDDT(sym_lib::CSR *L, sym_lib::CSC *L_csc,
           double *correct_x, DDT::Config &conf,
           std::string name) :
    SpMVSerial(L, L_csc, correct_x, name), config(conf), cl(nullptr) {
   L1_csr_ = L;
   L1_csc_ = L_csc;
   correct_x_ = correct_x;
  };

  sym_lib::timing_measurement get_analysis_bw(){return analysis_breakdown;}

  ~SpMVDDT(){
   DDT::free(d);
   delete[] cl;
  }
 };



}


#endif //SPARSE_AVX512_DEMO_SPMVVEC_H
