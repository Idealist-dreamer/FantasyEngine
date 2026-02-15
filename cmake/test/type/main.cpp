#include <gtest/gtest.h>
#include <benchmark/benchmark.h>

#include <engine/engine.h>
using namespace fe::engine;

#include "stlPerformance.h"
#include "mallocPtr.h"
#include "mallocPerformance.h"

int main(int argc, char* argv[]) {
  printf("=== Running Unit Tests ===\n");
  testing::InitGoogleTest(&argc, argv);
  int test_result = RUN_ALL_TESTS();

  if (test_result != 0) {
    printf("Tests failed, skipping benchmarks.\n");
    return test_result;
  }

  printf("\n=== Running Performance Benchmarks ===\n");
  benchmark::Initialize(&argc, argv);
  if (benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();

  return 0;
}