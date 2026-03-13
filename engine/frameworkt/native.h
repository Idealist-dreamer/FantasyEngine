#pragma once

#include "common.h"
#include "foundation/utility/assert.h"

namespace fe::engine {

// ============================================================================
// Entity access types
// ============================================================================

struct EntityQuery {
    EntityQuery(Registry& reg) : m_reg(reg) {}

    bool valid(Entity e) const { return m_reg.valid(e); }
    auto view() const { return m_reg.view<Entity>(); }

protected:
    Registry& m_reg;
};

struct EntityCreator : EntityQuery {
    EntityCreator(Registry& reg) : EntityQuery(reg) {}

    Entity create() { return m_reg.create(); }
    auto view() { return m_reg.view<Entity>(); }
};

struct EntityDestroyer : EntityCreator {
    EntityDestroyer(Registry& reg) : EntityCreator(reg) {}

    void destroy(Entity e) { m_reg.destroy(e); }
};

struct EntityCommandBuffer {
    using EntityHandle = uint32_t;

    EntityCommandBuffer() = default;

    EntityHandle create() {
        EntityHandle handle = static_cast<EntityHandle>(m_entity_map.size());
        m_entity_map.emplace_back(entt::null);
        return handle;
    }

    bool valid(EntityHandle handle) const {
        if (handle >= m_entity_map.size()) {
            return false;
        }
        return m_entity_map[handle] != entt::null;
    }

    Entity get(EntityHandle handle) const {
        if (handle >= m_entity_map.size()) {
            FE_ASSERT(false);
            return entt::null;
        }
        return m_entity_map[handle];
    }

    void destroy(Entity e) { m_destroyed_entities.push_back(e); }

    void clear() { m_entity_map.clear(); }

    stl::vector<Entity> m_entity_map;
    stl::vector<Entity> m_destroyed_entities;
};

// ============================================================================
// Component access types
// ============================================================================

template <typename... T>
using exclude_t = entt::exclude_t<T...>;

template <typename... T>
using get_t = entt::get_t<T...>;

template <typename... Components>
class ComponentReader {
    using ComponentTuple = std::tuple<Components...>;

public:
    template <typename T>
    static constexpr void check_auth() {
        static_assert((is_compatible<T, Components>::value || ...), "ECS Access Denied: Component or variant not authorized in pack.");
    }

    template <typename... Ts>
    static constexpr void check_auth_all() {
        (check_auth<Ts>(), ...);
    }

    explicit ComponentReader(Registry& reg) : m_reg(reg) {}

    // Subset conversion constructor
    template <typename... OtherComponents, typename OtherTuple = std::tuple<OtherComponents...>,
              typename = std::enable_if_t<std::is_same_v<ComponentTuple, OtherTuple> || 
                  (sizeof...(Components) <= sizeof...(OtherComponents) && 
                   (is_compatible<Components, OtherComponents>::value && ...))>>
    ComponentReader(const ComponentReader<OtherComponents...>& other) : m_reg(other.m_reg) {}

    template <typename T>
    bool have(Entity e) const {
        check_auth<T>();
        return m_reg.all_of<T>(e);
    }

    template <typename... T>
    bool have_all(Entity e) const {
        check_auth_all<T...>();
        return m_reg.all_of<T...>(e);
    }

    template <typename... T>
    bool have_any(Entity e) const {
        check_auth_all<T...>();
        return m_reg.any_of<T...>(e);
    }

    template <typename T>
    const T* try_get(Entity e) const {
        check_auth<T>();
        return m_reg.try_get<T>(e);
    }

    template <typename T>
    const T& get(Entity e) const {
        check_auth<T>();
        return m_reg.get<T>(e);
    }

    auto view() const { return m_reg.view<Components...>(); }

    template <typename... Req, typename = std::enable_if_t<(sizeof...(Req) > 0)>>
    auto view() const {
        (check_auth<Req>(), ...);
        return m_reg.view<Req...>();
    }

    template <typename... GetTypes, typename... ExcludeTypes>
    auto view(exclude_t<ExcludeTypes...>) const {
        check_auth_all<GetTypes...>();
        check_auth_all<ExcludeTypes...>();
        return m_reg.view<GetTypes...>(entt::exclude<ExcludeTypes...>);
    }

    template <typename... Owned, typename... Get, typename... Exclude>
    auto group(get_t<Get...> = {}, exclude_t<Exclude...> = {}) const {
        check_auth_all<Owned...>();
        check_auth_all<Get...>();
        check_auth_all<Exclude...>();
        return m_reg.group<Owned...>(entt::get<Get...>, entt::exclude<Exclude...>);
    }

protected:
    Registry& m_reg;

    // Allow different ComponentReader/Writer instantiations to access m_reg
    template <typename...>
    friend class ComponentReader;
    template <typename...>
    friend class ComponentWriter;
};

template <typename... Components>
class ComponentWriter : public ComponentReader<Components...> {
    using Base = ComponentReader<Components...>;
    using ComponentTuple = std::tuple<Components...>;

public:
    using Base::check_auth;
    using Base::check_auth_all;
    using Base::get;
    using Base::group;
    using Base::have;
    using Base::have_all;
    using Base::have_any;
    using Base::m_reg;
    using Base::try_get;
    using Base::view;

    explicit ComponentWriter(Registry& reg) : Base(reg) {}

    // Subset conversion constructor
    template <typename... OtherComponents, typename OtherTuple = std::tuple<OtherComponents...>,
              typename = std::enable_if_t<std::is_same_v<ComponentTuple, OtherTuple> ||
                  (sizeof...(Components) <= sizeof...(OtherComponents) &&
                   (is_compatible<Components, OtherComponents>::value && ...))>>
    ComponentWriter(ComponentWriter<OtherComponents...>& other) : Base(other.m_reg) {}

    template <typename... OtherComponents, typename OtherTuple = std::tuple<OtherComponents...>,
              typename = std::enable_if_t<std::is_same_v<ComponentTuple, OtherTuple> ||
                  (sizeof...(Components) <= sizeof...(OtherComponents) &&
                   (is_compatible<Components, OtherComponents>::value && ...))>>
    ComponentWriter(const ComponentWriter<OtherComponents...>& other) : Base(other.m_reg) {}

    template <typename T>
    T& get(Entity e) {
        check_auth<T>();
        return m_reg.get<T>(e);
    }

    template <typename T>
    T& get_or_emplace(Entity e) {
        check_auth<T>();
        return m_reg.get<T>(e);
    }

    auto view() { return m_reg.view<Components...>(); }

    template <typename... Req, typename = std::enable_if_t<(sizeof...(Req) > 0)>>
    auto view() {
        (check_auth<Req>(), ...);
        return m_reg.view<Req...>();
    }

    template <typename... Get, typename... Exclude>
    auto view(exclude_t<Exclude...>) {
        check_auth_all<Get...>();
        check_auth_all<Exclude...>();
        return m_reg.view<Get...>(entt::exclude<Exclude...>);
    }

    template <typename... Owned, typename... Get, typename... Exclude>
    auto group(get_t<Get...> = {}, exclude_t<Exclude...> = {}) {
        check_auth_all<Owned...>();
        check_auth_all<Get...>();
        check_auth_all<Exclude...>();
        return m_reg.group<Owned...>(entt::get<Get...>, entt::exclude<Exclude...>);
    }

    template <typename T, typename Compare>
    void sort(Compare comp) {
        check_auth<T>();
        m_reg.sort<T>(comp);
    }

    template <typename ToSort, typename SortBy>
    void sort() {
        check_auth_all<ToSort, SortBy>();
        m_reg.sort<ToSort, SortBy>();
    }

    template <typename T, typename... Args>
    T& add(Entity e, Args&&... args) {
        check_auth<T>();
        return m_reg.emplace_or_replace<T>(e, std::forward<Args>(args)...);
    }

    template <typename T>
    bool remove(Entity e) {
        check_auth<T>();
        return m_reg.remove<T>(e) > 0;
    }

    template <typename T>
    void clear() {
        check_auth<T>();
        m_reg.clear<T>();
    }

    template <typename T>
    void tag_add(Entity e) {
        add<AddTag<T>>(e);
    }

    template <typename T>
    void tag_change(Entity e) {
        add<ChangeTag<T>>(e);
    }

    template <typename T>
    void tag_remove(Entity e) {
        add<RemoveTag<T>>(e);
    }

    template <typename T, typename... Args>
    void add_delayed(Entity e, Args&&... args) {
        check_auth<T>();
        if (!have<T>(e)) {
            m_reg.remove<ChangeDelayed<T>, RemoveDelayed<T>>(e);
            m_reg.emplace_or_replace<AddDelayed<T>>(e, T{std::forward<Args>(args)...});
        }
    }

    template <typename T, typename... Args>
    void change_delayed(Entity e, Args&&... args) {
        check_auth<T>();
        if (m_reg.try_get<T>(e) && !m_reg.all_of<RemoveDelayed<T>>(e)) {
            m_reg.emplace_or_replace<ChangeDelayed<T>>(e, T{std::forward<Args>(args)...});
        } else if (auto* add_ptr = m_reg.try_get<AddDelayed<T>>(e)) {
            add_ptr->data = T(std::forward<Args>(args)...);
        }
    }

    template <typename T>
    void remove_delayed(Entity e) {
        check_auth<T>();
        if (m_reg.try_get<T>(e)) {
            m_reg.remove<AddDelayed<T>, ChangeDelayed<T>>(e);
            m_reg.emplace_or_replace<RemoveDelayed<T>>(e);
        }
    }
};

// ============================================================================
// Event access types
// ============================================================================

template <typename T>
class EventReader {
public:
    explicit EventReader(stl::vector<T>& events) : m_events(events) {}

    const stl::vector<T>& get() const { return m_events; }

protected:
    stl::vector<T>& m_events;
};

template <typename T>
class EventWriter : public EventReader<T> {
public:
    using EventReader<T>::get;
    using EventReader<T>::m_events;

    explicit EventWriter(stl::vector<T>& events) : EventReader<T>(events) {}

    stl::vector<T>& get() { return m_events; }
};

// ============================================================================
// Context access types
// ============================================================================

template <typename T>
struct ContextStorage {
    Any data;
};

template <typename T>
class ContextReader {
public:
    explicit ContextReader(Any& data) : m_data(data) {}

    bool valid() const { return m_data.valid(); }

    const T& get() const { return *m_data.get<T>(); }

protected:
    Any& m_data;
};

template <typename T>
class ContextWriter {
public:
    explicit ContextWriter(Any& data) : m_data(data) {}

    bool valid() const { return m_data.valid(); }

    T& get() { return *m_data.get<T>(); }
    const T& get() const { return *m_data.get<T>(); }

    void destroy() { m_data.destroy(); }

    template <typename... Args>
    void create(Args&&... args) {
        m_data = Any::create<T>(std::forward<Args>(args)...);
    }

private:
    Any& m_data;
};

// ============================================================================
// Double buffer for events
// ============================================================================

template <typename T>
struct DoubleBuffer {
    stl::vector<T> read_buf;
    stl::vector<T> write_buf;

    void swap() {
        read_buf.swap(write_buf);
        write_buf.clear();
    }
};

} // namespace fe::engine
