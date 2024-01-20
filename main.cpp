#include "Analyzer.h"
#include "DDT.h"
#include "Executor.h"
#include "GenerateCppSource.h"
#include "Input.h"
#include "Inspector.h"

#include <DDTUtils.h>
#include <algorithm>
#include <sys/time.h>

int main(int argc, char** argv) {
  // Parse program arguments
  auto config = DDT::parseInput(argc, argv);

  // Allocate memory and generate global object
  auto d = DDT::init(config);

  // Parse into run-time Codelets
  auto cl = new std::vector<DDT::Codelet *>[config.nThread]();
    if (config.bench_inspector) {
    struct timeval it1;
    gettimeofday(&it1, NULL);
    long it1s = it1.tv_sec * 1000000L + it1.tv_usec;
  DDT::inspectSerialTrace(d, cl, config);
    struct timeval it2;
    gettimeofday(&it2, NULL);
    long it2s = it2.tv_sec * 1000000L + it2.tv_usec;
    std::cout << (it2s - it1s) << std::endl;
    }
    else {
      DDT::inspectSerialTrace(d, cl, config);
    }
  // Execute codes
  if (config.analyze) {
      DDT::analyzeData(d, cl, config);
  } else {
      DDT::executeCodelets(cl, config);
      // DDT::generateSource(d);
  }

  // Clean up
  DDT::free(d);

  return 0;
}
