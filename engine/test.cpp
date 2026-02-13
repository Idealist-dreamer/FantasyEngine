#include "test.h"

#include <iostream>

EngineTest::EngineTest(int _a) : a(_a) {}

void EngineTest::Print() {
  std::cout << "a = " << a << std::endl;
}