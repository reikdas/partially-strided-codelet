//
// Created by cetinicz on 2021-07-06.
//

#ifndef DDT_PATTERNMATCHING_H
#define DDT_PATTERNMATCHING_H

#include "DDT.h"

#include <chrono>

namespace DDT {
    void computeFirstOrder(int *differences, int *tuples, int numTuples);

    void computeParallelizedFOD(int **ip, int ips, int *differences,
                                int nThread);

    uint16_t generateDifferenceMask(int *lhs, int *mid, int *rhs, int MASK);

    void mineDifferences(int **ip, DDT::PatternDAG *c, int *d, int nThreads,
                         const int *tBound);

    double getTimeDifference(std::chrono::steady_clock::time_point t1,
                             std::chrono::steady_clock::time_point t2);

    int findCLCS(int tpd, int *lhstp, int *rhstp, int lhstps, int rhstps,
                 DDT::PatternDAG *lhscp, DDT::PatternDAG *rhscp, int *lhstpd,
                 int *rhstpd);
}

#endif  // DDT_PATTERNMATCHING_H
