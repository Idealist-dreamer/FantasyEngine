#pragma once

#include "common.h"
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/map.hpp>
#include <iostream>
#include <variant>

namespace fe::engine {

#define FE_MAKE_NVP(val) ::cereal::make_nvp(#val, val)
#define FE_MAKE_BINARY_DATA(ptr, size) ::cereal::binary_data(ptr, size)

using AnyArchive = std::variant<
    cereal::BinaryOutputArchive*,
    cereal::JSONOutputArchive*,
    cereal::BinaryInputArchive*,
    cereal::JSONInputArchive*
>;

// Archive wrapper for entity/component serialization
class Archive {
public:
    explicit Archive(AnyArchive ar, Registry* reg = nullptr) 
        : m_ar(ar), m_reg(reg) {
        if (m_reg) {
            if (is_output()) {
                m_snapshot = std::make_unique<entt::snapshot>(*m_reg);
            } else {
                m_loader = std::make_unique<entt::snapshot_loader>(*m_reg);
            }
        }
    }

    template<typename... Args>
    bool operator()(Args&&... args) {
        try {
            std::visit([&](auto* ar) { (*ar)(std::forward<Args>(args)...); }, m_ar);
            return true;
        } catch (const cereal::Exception&) {
            return false;
        }
    }

    void entities() {
        if (!m_reg) return;
        std::visit([this](auto* ar) {
            using ArType = std::remove_pointer_t<decltype(ar)>;
            if constexpr (std::is_same_v<ArType, cereal::BinaryOutputArchive> || 
                          std::is_same_v<ArType, cereal::JSONOutputArchive>) {
                m_snapshot->template get<entt::entity>(*ar);
            } else {
                m_loader->template get<entt::entity>(*ar);
            }
        }, m_ar);
    }

    template<typename... Components>
    void components() {
        if (!m_reg) return;
        std::visit([this](auto* ar) {
            using ArType = std::remove_pointer_t<decltype(ar)>;
            if constexpr (std::is_same_v<ArType, cereal::BinaryOutputArchive> || 
                          std::is_same_v<ArType, cereal::JSONOutputArchive>) {
                ((m_snapshot->template get<Components>(*ar)), ...);
            } else {
                ((m_loader->template get<Components>(*ar)), ...);
            }
        }, m_ar);
    }

    bool is_input() const { return m_ar.index() >= 2; }
    bool is_output() const { return m_ar.index() < 2; }
    bool is_json() const { return m_ar.index() == 1 || m_ar.index() == 3; }
    bool is_binary() const { return !is_json(); }

private:
    AnyArchive m_ar;
    Registry* m_reg = nullptr;
    std::unique_ptr<entt::snapshot> m_snapshot;
    std::unique_ptr<entt::snapshot_loader> m_loader;
};

} // namespace fe::engine

#ifdef FE_USE_EASTL
#include "foundation/container/stl.h"

namespace cereal {
using size_type = uint64_t;

// eastl::string serialization
template<class Archive, class T, class Allocator>
void save(Archive& ar, eastl::basic_string<T, Allocator> const& str) {
    if constexpr (std::is_same_v<Archive, cereal::BinaryOutputArchive>) {
        ar(make_size_tag(static_cast<size_type>(str.size())));
        ar(binary_data(str.data(), str.size() * sizeof(T)));
    } else {
        std::basic_string<T> stdStr(str.begin(), str.end());
        ar(stdStr);
    }
}

template<class Archive, class T, class Allocator>
void load(Archive& ar, eastl::basic_string<T, Allocator>& str) {
    if constexpr (std::is_same_v<Archive, cereal::BinaryInputArchive>) {
        size_type size;
        ar(make_size_tag(size));
        str.resize(static_cast<size_t>(size));
        ar(binary_data(str.data(), static_cast<size_t>(size) * sizeof(T)));
    } else {
        std::basic_string<T> stdStr;
        ar(stdStr);
        str.assign(stdStr.c_str(), stdStr.length());
    }
}

// eastl::vector serialization
template<class Archive, class T, class Allocator>
void save(Archive& ar, eastl::vector<T, Allocator> const& vec) {
    ar(make_size_tag(static_cast<size_type>(vec.size())));
    if constexpr (std::is_trivially_copyable_v<T> && 
                  traits::is_output_serializable<BinaryData<T>, Archive>::value) {
        ar(binary_data(vec.data(), vec.size() * sizeof(T)));
    } else {
        for (auto const& i : vec) ar(i);
    }
}

template<class Archive, class T, class Allocator>
void load(Archive& ar, eastl::vector<T, Allocator>& vec) {
    size_type size;
    ar(make_size_tag(size));
    vec.resize(static_cast<size_t>(size));
    if constexpr (std::is_trivially_copyable_v<T> && 
                  traits::is_input_serializable<BinaryData<T>, Archive>::value) {
        ar(binary_data(vec.data(), static_cast<size_t>(size) * sizeof(T)));
    } else {
        for (auto& i : vec) ar(i);
    }
}

// eastl::map serialization
template<class Archive, typename... Args>
void save(Archive& ar, eastl::map<Args...> const& map) {
    ar(make_size_tag(static_cast<size_type>(map.size())));
    for (auto const& i : map) ar(i);
}

template<class Archive, typename... Args>
void load(Archive& ar, eastl::map<Args...>& map) {
    size_type size;
    ar(make_size_tag(size));
    map.clear();
    for (size_type i = 0; i < size; ++i) {
        typename eastl::map<Args...>::value_type vt;
        ar(vt);
        map.insert(eastl::move(vt));
    }
}

} // namespace cereal
#endif
