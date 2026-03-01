#pragma once

#include "common.h"

namespace fe::engine::ecs {
template <typename T>
class EventReader {
 public:
  EventReader(const stl::vector<T>& _events) : m_events(_events) {}

  const stl::vector<T>& get() const { return m_events; }

 private:
  const stl::vector<T>& m_events;
};

template <typename T>
class EventWriter {
 public:
  EventWriter(stl::vector<T>& _events) : m_events(_events) {}

  stl::vector<T>& get() const { return m_events; }

 private:
  stl::vector<T>& m_events;
};
}  // namespace fe::engine::ecs