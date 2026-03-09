#pragma once

namespace fe::engine {

template <typename T>
class Visitor {
  T& obj;

 public:
  Visitor(T& _obj) : obj(_obj) {}

  Visitor(const Visitor& other) = delete;
  Visitor& operator=(const Visitor& other) = delete;

  T& get() { return obj; }
  const T& get() const { return obj; }

  T* operator->() { return &obj; }
  const T* operator->() const { return &obj; }
};

}  // namespace fe::engine