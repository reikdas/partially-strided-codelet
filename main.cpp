#include "DDT.h"
#include "Input.h"
#include "Inspector.h"
#include "PatternMatching.h"

#include <iostream>

/**
 *
 * Different iteration sections processed differently
 *
 * 1) Prune iterations completely out of processing
 * 2) Mark single iterations
 * 3) Something to enable PSC (?)
 * 4) Di
 */
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
  std::cout << "Pruning Time: " << getTimeDifference(t0,t1) << std::endl;
}

int main(int argc, char** argv) {
  // Parse program arguments
  auto config = DDT::parseInput(argc, argv);

  // Allocate memory and generate global object
  auto d = DDT::init(config);

  // Prune memory trace
  pruneIterations(d.mt, 10);

  // Compute and Mark regions for PSC-3
  computeParallelizedFOD(d.mt.ip, d.mt.ips, d.d);

  // Mine trace and profile
  mineDifferences(d.mt.ip, d.mt.ips, d.c, d.d);

  // Generate Codes
  // DDT::generateSource(d);
  std::vector<DDT::Codelet*> cl;
  DDT::inspectCodelets(d, cl);

  for (auto const& c : cl) {
      std::cout << c->get_type() << std::endl;
  }

  // Clean up
  DDT::free(d);

  return 0;
}
