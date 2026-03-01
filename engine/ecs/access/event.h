#pragma once

#include "common.h"

namespace fe::engine::ecs {
template <typename T>
class EventReader {
 public:
  ResourceReader(const stl::vector<T>& _events) : events(_events) {}

  const stl::vector<T>& get() const { return events; }

 private:
  const stl::vector<T>& events;
};

template <typename T>
class EventWriter {
 public:
  EventWriter(stl::vector<T>& _events) : events(_events) {}

  stl::vector<T>& get() const { return events; }

 private:
  stl::vector<T>& events;
};
}  // namespace fe::engine::ecs