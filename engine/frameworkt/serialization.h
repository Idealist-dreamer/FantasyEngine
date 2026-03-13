#pragma once

#include "common.h"
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/map.hpp>
#include <variant>
#include <iostream>

namespace fe::engine {

#define FE_MAKE_NVP(val) ::cereal::make_nvp(#val, val)
#define FE_MAKE_BINARY_DATA(ptr, size) ::cereal::binary_data(ptr, size)

using AnyArchive = std::variant<
    cereal::BinaryOutputArchive*,
    cereal::JSONOutputArchive*,
    cereal::BinaryInputArchive*,
    cereal::JSONInputArchive*
>;

// 档案包装器，支持实体快照
class Archive {
public:
    explicit Archive(AnyArchive ar, Registry* reg = nullptr)
        : m_ar(ar), m_reg(reg) {
        if (m_reg) {
            if (is_output())
                m_snapshot = std::make_unique<entt::snapshot>(*m_reg);
            else
                m_loader = std::make_unique<entt::snapshot_loader>(*m_reg);
        }
    }

    template <typename... Args>
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

    template <typename... Components>
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

private:
    AnyArchive m_ar;
    Registry* m_reg = nullptr;
    std::unique_ptr<entt::snapshot> m_snapshot;
    std::unique_ptr<entt::snapshot_loader> m_loader;
};

} // namespace fe::engine

#ifdef FE_USE_EASTL
#include <cereal/cereal.hpp>
#include <cereal/details/helpers.hpp>
#include "foundation/container/stl.h"

namespace cereal {

using size_type = uint64_t;

// eastl::string serialization
template <class Archive, class T, class Allocator>
inline void save(Archive& ar, eastl::basic_string<T, Allocator> const& str) {
    if constexpr (std::is_same_v<Archive, cereal::BinaryOutputArchive>) {
        ar(make_size_tag(static_cast<size_type>(str.size())));
        ar(binary_data(str.data(), str.size() * sizeof(T)));
    } else {
        std::basic_string<T> stdStr(str.begin(), str.end());
        ar(stdStr);
    }
}

template <class Archive, class T, class Allocator>
inline void load(Archive& ar, eastl::basic_string<T, Allocator>& str) {
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

template <class Archive, class T, class Allocator>
inline void save(Archive& ar, eastl::vector<T, Allocator> const& vector) {
    ar(make_size_tag(static_cast<size_type>(vector.size())));
    if constexpr (std::is_trivially_copyable_v<T> && traits::is_output_serializable<BinaryData<T>, Archive>::value) {
        ar(binary_data(vector.data(), vector.size() * sizeof(T)));
    } else {
        for (auto const& i : vector)
            ar(i);
    }
}

template <class Archive, class T, class Allocator>
inline void load(Archive& ar, eastl::vector<T, Allocator>& vector) {
    size_type size;
    ar(make_size_tag(size));
    vector.resize(static_cast<size_t>(size));
    if constexpr (std::is_trivially_copyable_v<T> && traits::is_input_serializable<BinaryData<T>, Archive>::value) {
        ar(binary_data(vector.data(), static_cast<size_t>(size) * sizeof(T)));
    } else {
        for (auto& i : vector)
            ar(i);
    }
}

template <class Archive, typename... Args>
inline void save(Archive& ar, eastl::map<Args...> const& container) {
    ar(make_size_tag(static_cast<size_type>(container.size())));
    for (auto const& i : container)
        ar(i);
}

template <class Archive, typename... Args>
inline void load(Archive& ar, eastl::map<Args...>& container) {
    size_type size;
    ar(make_size_tag(size));
    container.clear();
    for (size_t i = 0; i < size; ++i) {
        typename eastl::map<Args...>::value_type vt;
        ar(vt);
        container.insert(eastl::move(vt));
    }
}

template <class Archive, class T, class D>
inline void serialize(Archive& ar, eastl::unique_ptr<T, D>& ptr) {
    T* rawPtr = ptr.get();
    ar(rawPtr);
    if (Archive::is_loading::value)
        ptr.reset(rawPtr);
}

} // namespace cereal
#endif
