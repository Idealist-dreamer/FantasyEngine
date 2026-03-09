#pragma once

#include "external.h"

namespace fe::engine {

struct Asset {
  Asset(const stl::string& file_path = "") : m_file_path(file_path) {}

  const stl::string get_file_path() const { return m_file_path; }
  void set_file_path(const stl::string& file_path) { m_file_path = file_path; }

  bool is_load() const { return m_is_loaded; }

  template <typename T>
  void set_load(T* obj) {
    reset();
    m_load_type_hash = std::type_index(typeid(T)).hash_code();
    m_load_obj = static_cast<void*>(obj);
    m_is_loaded = true;
  }

  template <typename T>
  T* get_load() const {
    if (m_is_loaded && m_load_type_hash == std::type_index(typeid(T)).hash_code()) {
      return static_cast<T*>(m_load_obj);
    }
    return nullptr;
  }

  void reset() {
    m_load_type_hash = std::type_index(typeid(void)).hash_code();
    m_load_obj = nullptr;
    m_is_loaded = false;
  }

 private:
  stl::string m_file_path;
  bool m_is_loaded = false;
  void* m_load_obj = nullptr;
  uint64_t m_load_type_hash = 0;
};

enum struct AssetType { unKnown = 0, gltf, shader };

struct AssetHandle {
  AssetType type;
  uint32_t id;
};

class AssetManager {
  static inline uint32_t s_next_id = 0;

 public:
  AssetManager() = default;
  ~AssetManager() = default;

  bool have_asset(AssetHandle handle) const {
    return m_asset_map.find(handle.type) != m_asset_map.end() && m_asset_map.at(handle.type).find(handle.id) != m_asset_map.at(handle.type).end();
  }

  AssetHandle add_asset(AssetType type, Asset asset) {
    auto& asset_map = m_asset_map[type];
    auto value = s_next_id++;
    asset_map.insert({value, asset});
    return {type, value};
  }

  Asset* get_asset(AssetHandle handle) {
    if (!have_asset(handle)) {
      return nullptr;
    }

    return &(m_asset_map.at(handle.type).at(handle.id));
  }

  stl::unordered_map<uint32_t, Asset>& get_type_asset(AssetType type) { return m_asset_map.at(type); }

  bool remove_asset(AssetHandle handle) {
    if (have_asset(handle)) {
      m_asset_map[handle.type].erase(m_asset_map[handle.type].find(handle.id));
      return true;
    }
    return false;
  }

 private:
  stl::unordered_map<AssetType, stl::unordered_map<uint32_t, Asset>> m_asset_map;
};
}  // namespace fe::engine