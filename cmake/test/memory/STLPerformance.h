// #pragma once

// using namespace fe::engine;

// #include <queue>

// // 1. Vector: 动态扩容与重分配压力
// template <typename V>
// static void BM_Vector_PushBack(benchmark::State& state) {
//   for (auto _ : state) {
//     V v;
//     for (int i = 0; i < state.range(0); ++i)
//       v.push_back(i);
//     benchmark::DoNotOptimize(v.data());
//   }
// }
// BENCHMARK_TEMPLATE(BM_Vector_PushBack, std::vector<int>)->Range(128, 8192)->Name("BM_Std_Vector_PushBack");
// BENCHMARK_TEMPLATE(BM_Vector_PushBack, stl::vector<int>)->Range(128, 8192)->Name("BM_Stl_Vector_PushBack");
// BENCHMARK_TEMPLATE(BM_Vector_PushBack, vector<int>)->Range(128, 8192)->Name("BM_GAlloc_Vector_PushBack");

// // 2. Map: 关联容器节点分配压力 (红黑树)
// template <typename M>
// static void BM_Map_Insert(benchmark::State& state) {
//   for (auto _ : state) {
//     M m;
//     for (int i = 0; i < 1000; ++i)
//       m[i] = i;
//     benchmark::DoNotOptimize(m);
//   }
// }
// BENCHMARK_TEMPLATE(BM_Map_Insert, std::map<int, int>)->Name("BM_Std_Map_Insert");
// BENCHMARK_TEMPLATE(BM_Map_Insert, stl::map<int, int>)->Name("BM_Stl_Map_Insert");
// BENCHMARK_TEMPLATE(BM_Map_Insert, map<int, int>)->Name("BM_GAlloc_Map_Insert");

// // 3. UnorderedMap: 哈希开销与桶分配压力
// template <typename UM>
// static void BM_UnorderedMap_Insert(benchmark::State& state) {
//   for (auto _ : state) {
//     UM m;
//     for (int i = 0; i < 1000; ++i)
//       m[i] = i;
//     benchmark::DoNotOptimize(m);
//   }
// }
// BENCHMARK_TEMPLATE(BM_UnorderedMap_Insert, std::unordered_map<int, int>)->Name("BM_Std_UnorderedMap_Insert");
// BENCHMARK_TEMPLATE(BM_UnorderedMap_Insert,
//                    stl::hash_map<int, int, stl::hash<int>, stl::equal_to<int>, StlAllocator<AllocType::STD, stl::pair<const int, int>>>)
//     ->Name("BM_Stl_UnorderedMap_Insert");
// BENCHMARK_TEMPLATE(BM_UnorderedMap_Insert, unordered_map<int, int>)->Name("BM_GAlloc_UnorderedMap_Insert");

// // 4. String: 频繁重分配与长字符串处理
// template <typename S>
// static void BM_String_Append(benchmark::State& state) {
//   S base = "Revolution_Engine_Test_String_Base_Long_Prefix_";
//   for (auto _ : state) {
//     S s = base;
//     for (int i = 0; i < 50; ++i)
//       s += "append_extra_data_to_force_growth_and_reallocation_";
//     benchmark::DoNotOptimize(s);
//   }
// }
// BENCHMARK_TEMPLATE(BM_String_Append, std::string)->Name("BM_Std_String_Append");
// BENCHMARK_TEMPLATE(BM_String_Append, stl::string)->Name("BM_Stl_String_Append");
// BENCHMARK_TEMPLATE(BM_String_Append, string)->Name("BM_GAlloc_String_Append");

// // 5. List: 链表节点频繁分配与释放 (Cache Locality 测试)
// template <typename L>
// static void BM_List_Cycle(benchmark::State& state) {
//   for (auto _ : state) {
//     L l;
//     for (int i = 0; i < 1000; ++i)
//       l.push_back(i);
//     l.clear();
//     benchmark::DoNotOptimize(l);
//   }
// }
// BENCHMARK_TEMPLATE(BM_List_Cycle, std::list<int>)->Name("BM_Std_List_Cycle");
// BENCHMARK_TEMPLATE(BM_List_Cycle, stl::list<int>)->Name("BM_Stl_List_Cycle");
// BENCHMARK_TEMPLATE(BM_List_Cycle, list<int>)->Name("BM_GAlloc_List_Cycle");

// // 6. Deque/Stack: 块状分配测试 (Queue/Stack 基于 Deque)
// template <typename Q>
// static void BM_Queue_PushPop(benchmark::State& state) {
//   for (auto _ : state) {
//     Q q;
//     for (int i = 0; i < 1000; ++i)
//       q.push(i);
//     while (!q.empty())
//       q.pop();
//     benchmark::DoNotOptimize(q);
//   }
// }
// BENCHMARK_TEMPLATE(BM_Queue_PushPop, std::queue<int>)->Name("BM_Std_Queue_Cycle");
// BENCHMARK_TEMPLATE(BM_Queue_PushPop, stl::queue<int>)->Name("BM_Stl_Queue_Cycle");
// BENCHMARK_TEMPLATE(BM_Queue_PushPop, queue<int>)->Name("BM_GAlloc_Queue_Cycle");