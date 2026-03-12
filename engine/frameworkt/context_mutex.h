#pragma once

#include "foundation/container/stl.h"

namespace fe::engine {

struct AccessFlags {
  stl::unordered_set<size_t> reads;
  stl::unordered_set<size_t> writes;
};

struct ContextMutex {
  bool exclusive = false;
  stl::unordered_map<size_t, AccessFlags> context_to_flags;

  template <typename ContextT, typename TagT>
  void add_read() {
    context_to_flags[typeid(ContextT).hash_code()].reads.insert(typeid(TagT).hash_code());
  }

  template <typename ContextT, typename TagT>
  void add_write() {
    context_to_flags[typeid(ContextT).hash_code()].writes.insert(typeid(TagT).hash_code());
  }

  void merge(const ContextMutex& other) {
    if (other.exclusive)
      exclusive = true;
    for (const auto& [ctx, flags] : other.context_to_flags) {
      context_to_flags[ctx].reads.insert(flags.reads.begin(), flags.reads.end());
      context_to_flags[ctx].writes.insert(flags.writes.begin(), flags.writes.end());
    }
  }

  bool is_conflict(const ContextMutex& other) const {
    if (exclusive || other.exclusive)
      return true;

    for (const auto& [ctx, flags1] : context_to_flags) {
      auto it = other.context_to_flags.find(ctx);
      if (it == other.context_to_flags.end())
        continue;

      const auto& flags2 = it->second;

      if (has_intersection(flags1.reads, flags2.writes))
        return true;

      if (has_intersection(flags1.writes, flags2.reads))
        return true;

      if (has_intersection(flags1.writes, flags2.writes))
        return true;
    }
    return false;
  }

 private:
  static bool has_intersection(const stl::unordered_set<size_t>& set1, const stl::unordered_set<size_t>& set2) {
    const auto& smaller = (set1.size() < set2.size()) ? set1 : set2;
    const auto& larger = (set1.size() < set2.size()) ? set2 : set1;
    for (const auto& item : smaller) {
      if (larger.find(item) != larger.end())
        return true;
    }
    return false;
  }
};

}  // namespace fe::engine