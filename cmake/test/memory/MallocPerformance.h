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
// 1. 基础分配吞吐量测试 (Malloc vs MiMalloc)
// ============================================================================

static void BM_StdMalloc(benchmark::State& state) {
  size_t size = state.range(0);
  for (auto _ : state) {
    void* p = ::malloc(size);
    benchmark::DoNotOptimize(p);
    ::free(p);
  }
}
BENCHMARK(BM_StdMalloc)->Range(8, 8192);

static void BM_MiMalloc(benchmark::State& state) {
  size_t size = state.range(0);
  for (auto _ : state) {
    void* p = MiAlloc::malloc(size);
    benchmark::DoNotOptimize(p);
    MiAlloc::free(p);
  }
}
BENCHMARK(BM_MiMalloc)->Range(8, 8192);

// ============================================================================
// 2. 单对象创建对比 (New vs Create)
// ============================================================================

static void BM_StdNew(benchmark::State& state) {
  for (auto _ : state) {
    SmallObj* p = new SmallObj();
    benchmark::DoNotOptimize(p);
    delete p;
  }
}
BENCHMARK(BM_StdNew);

static void BM_MiCreate(benchmark::State& state) {
  for (auto _ : state) {
    SmallObj* p = MiAlloc::create<SmallObj>();
    benchmark::DoNotOptimize(p);
    MiAlloc::destroy(p);
  }
}
BENCHMARK(BM_MiCreate);

// ============================================================================
// 3. 数组分配测试：真实负载 (强制写入以防止编译器优化掉循环)
// ============================================================================

static void BM_StdArray_RealWork(benchmark::State& state) {
  size_t count = state.range(0);
  for (auto _ : state) {
    SmallObj* p = new SmallObj[count];
    for (size_t i = 0; i < count; ++i)
      p[i].data[0] = i;  // 强制写入
    benchmark::DoNotOptimize(p);
    delete[] p;
  }
}
BENCHMARK(BM_StdArray_RealWork)->Arg(10)->Arg(100)->Arg(1000);

static void BM_MiCreateArray_RealWork(benchmark::State& state) {
  size_t count = state.range(0);
  for (auto _ : state) {
    SmallObj* p = MiAlloc::createArray<SmallObj>(count);
    for (size_t i = 0; i < count; ++i)
      p[i].data[0] = i;  // 强制写入
    benchmark::DoNotOptimize(p);
    MiAlloc::destroyArray(p, count);
  }
}
BENCHMARK(BM_MiCreateArray_RealWork)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// 4. 多线程竞争分配测试 (Multi-Thread Scaling)
// ============================================================================

static void BM_MT_StdMalloc(benchmark::State& state) {
  for (auto _ : state) {
    void* p = ::malloc(64);
    benchmark::DoNotOptimize(p);
    ::free(p);
  }
}
BENCHMARK(BM_MT_StdMalloc)->ThreadRange(1, 16);

static void BM_MT_MiMalloc(benchmark::State& state) {
  for (auto _ : state) {
    void* p = MiAlloc::malloc(64);
    benchmark::DoNotOptimize(p);
    MiAlloc::free(p);
  }
}
BENCHMARK(BM_MT_MiMalloc)->ThreadRange(1, 16);

// ============================================================================
// 5. 对齐分配性能测试 (Aligned Alloc)
// ============================================================================

static void BM_StdAlignedMalloc(benchmark::State& state) {
  for (auto _ : state) {
    // Windows 环境下标准对齐分配
    void* p = _aligned_malloc(1024, 64);
    benchmark::DoNotOptimize(p);
    _aligned_free(p);
  }
}
BENCHMARK(BM_StdAlignedMalloc);

static void BM_MiAlignedMalloc(benchmark::State& state) {
  for (auto _ : state) {
    void* p = MiAlloc::mallocAligned(1024, 64);
    benchmark::DoNotOptimize(p);
    MiAlloc::free(p);
  }
}
BENCHMARK(BM_MiAlignedMalloc);

// ============================================================================
// 6. 内存碎片化/乱序分配压力测试 (Memory Churn)
// ============================================================================

static void BM_MemoryChurn_Std(benchmark::State& state) {
  const int batch_size = 500;
  std::vector<void*> ptrs(batch_size);
  for (auto _ : state) {
    for (int i = 0; i < batch_size; ++i)
      ptrs[i] = ::malloc(64);
    // 乱序释放模拟碎片
    for (int i = 0; i < batch_size; i += 2)
      ::free(ptrs[i]);
    for (int i = 1; i < batch_size; i += 2)
      ::free(ptrs[i]);
  }
}
BENCHMARK(BM_MemoryChurn_Std);

static void BM_MemoryChurn_Mi(benchmark::State& state) {
  const int batch_size = 500;
  std::vector<void*> ptrs(batch_size);
  for (auto _ : state) {
    for (int i = 0; i < batch_size; ++i)
      ptrs[i] = MiAlloc::malloc(64);
    // 乱序释放模拟碎片
    for (int i = 0; i < batch_size; i += 2)
      MiAlloc::free(ptrs[i]);
    for (int i = 1; i < batch_size; i += 2)
      MiAlloc::free(ptrs[i]);
  }
}
BENCHMARK(BM_MemoryChurn_Mi);

// ============================================================================
// 7. 智能指针性能测试 (Smart Pointers)
// ============================================================================

// --- Unique Ptr ---
// static void BM_StdMakeUnique(benchmark::State& state) {
//   for (auto _ : state) {
//     auto p = std::make_unique<SmallObj>(42);
//     benchmark::DoNotOptimize(p);
//   }
// }
// BENCHMARK(BM_StdMakeUnique);

// static void BM_MiMakeUnique(benchmark::State& state) {
//   for (auto _ : state) {
//     auto p = MiAlloc::make_unique<SmallObj>(42);
//     benchmark::DoNotOptimize(p);
//   }
// }
// BENCHMARK(BM_MiMakeUnique);

// // --- Shared Ptr ---
// static void BM_StdMakeShared(benchmark::State& state) {
//   for (auto _ : state) {
//     auto p = std::make_shared<SmallObj>(42);
//     benchmark::DoNotOptimize(p);
//   }
// }
// BENCHMARK(BM_StdMakeShared);

// static void BM_MiMakeShared(benchmark::State& state) {
//   for (auto _ : state) {
//     auto p = MiAlloc::make_shared<SmallObj>(42);
//     benchmark::DoNotOptimize(p);
//   }
// }
// BENCHMARK(BM_MiMakeShared);