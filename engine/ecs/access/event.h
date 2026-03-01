#pragma once

#include "common.h"
#include "engine/base/pch.h"

namespace fe::engine::ecs {
template <typename T>
class EventReader {
 public:
  EventReader(stl::vector<T>& _events) : m_events(_events) {}

  const stl::vector<T>& get() const { return m_events; }

 protected:
  stl::vector<T>& m_events;
};

template <typename T>
class EventWriter : public EventReader<T> {
 public:
  using EventReader<T>::get;
  using EventReader<T>::m_events;

  EventWriter(stl::vector<T>& _events) : EventReader<T>(_events) {}

  stl::vector<T>& get() { return m_events; }
};
}  // namespace fe::engine::ecs

namespace fe::engine::ecs {
template <typename T>
struct is_event_reader : std::false_type {};
template <typename T>
struct is_event_reader<EventReader<T>> : std::true_type {
  using Type = T;
};

template <typename T>
struct is_event_writer : std::false_type {};
template <typename T>
struct is_event_writer<EventWriter<T>> : std::true_type {
  using Type = T;
};
}  // namespace fe::engine::ecs