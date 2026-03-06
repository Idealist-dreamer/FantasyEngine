#pragma once

#include "engine/base/pch.h"

namespace fe::engine::ecs {
enum struct AssetType { unKnown = 0, gltf, shader };

struct AssetId {
  AssetType type;
  uint32_t value;
};

struct Asset {
  Asset(const stl::string& file_path = "") : m_file_path(file_path) {}

  const stl::string get_file_path() const { return m_file_path; }
  void set_file_path(const stl::string& file_path) { m_file_path = file_path; }

  bool is_load() const { return m_is_loaded; }

  template <typename T>
  void load(T* obj) {
    free();
    m_load_type_hash = std::type_index(typeid(T)).hash_code();
    m_load_obj = static_cast<void*>(obj);
    m_is_loaded = true;
  }

  template <typename T>
  T* get() const {
    if (m_is_loaded && m_load_type_hash == std::type_index(typeid(T)).hash_code()) {
      return static_cast<T*>(m_load_obj);
    }
    return nullptr;
  }

  void free() {
    m_load_type_hash = std::type_index(typeid(void)).hash_code();
    m_load_obj = nullptr;
    m_is_loaded = false;
  }

 private:
  stl::string m_file_path;
  bool m_is_loaded = false;
  uint64_t m_load_type_hash = 0;
  void* m_load_obj = nullptr;
};

class AssetManager {
 public:
  AssetManager() = default;
  ~AssetManager() = default;

  bool have_asset(AssetId handle) const {
    return m_asset_map.find(handle.type) != m_asset_map.end() && m_asset_map.at(handle.type).find(handle.value) != m_asset_map.at(handle.type).end();
  }

  Asset& get_asset(AssetId handle) {
    FE_ASSERT(have_asset(handle));
    return m_asset_map.at(handle.type).at(handle.value);
  }

  stl::unordered_map<uint32_t, Asset>& get_type_asset(AssetType type) { return m_asset_map.at(type); }

  AssetId add_asset(AssetType type, Asset asset) {
    auto& asset_map = m_asset_map[type];
    auto value = static_cast<uint32_t>(asset_map.size());
    asset_map.insert({value, asset});
    return {type, value};
  }

  void change_asset(AssetId handle, Asset asset) {
    FE_ASSERT(have_asset(handle));
    m_asset_map.at(handle.type).at(handle.value) = asset;
  }

 private:
  stl::unordered_map<AssetType, stl::unordered_map<uint32_t, Asset>> m_asset_map;
};
}  // namespace fe::engine::ecs