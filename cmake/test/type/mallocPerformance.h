#pragma once

FE_NAME_ALLOCATOR(MiAlloc, MiMalloc);

// 测试用的典型对象：包含数据以防止编译器完全忽略构造
struct SmallObj {
  uint64_t data[2];
  SmallObj() {
    data[0] = 0;
    data[1] = 0;
  }
  SmallObj(uint64_t v) {
    data[0] = v;
    data[1] = v;
  }
};

// ============================================================================
// 7. 智能指针性能测试 (Smart Pointers)
// ============================================================================

// --- Unique Ptr ---
static void BM_StdMakeUnique(benchmark::State& state) {
  for (auto _ : state) {
    auto p = std::make_unique<SmallObj>(42);
    benchmark::DoNotOptimize(p);
  }
}
BENCHMARK(BM_StdMakeUnique);

static void BM_MiMakeUnique(benchmark::State& state) {
  for (auto _ : state) {
    auto p = make_unique<SmallObj>(42);
    benchmark::DoNotOptimize(p);
  }
}
BENCHMARK(BM_MiMakeUnique);

// --- Shared Ptr ---
static void BM_StdMakeShared(benchmark::State& state) {
  for (auto _ : state) {
    auto p = std::make_shared<SmallObj>(42);
    benchmark::DoNotOptimize(p);
  }
}
BENCHMARK(BM_StdMakeShared);

static void BM_MiMakeShared(benchmark::State& state) {
  for (auto _ : state) {
    auto p = make_shared<SmallObj>(42);
    benchmark::DoNotOptimize(p);
  }
}
BENCHMARK(BM_MiMakeShared);