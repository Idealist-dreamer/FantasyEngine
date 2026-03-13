#pragma once

#include "access.h"
#include "blackboard.h"
#include "native.h"

namespace fe::engine {

// 默认提供器：对于普通类型，直接以写模式访问
template <typename T>
struct ParamProvider {
    static void build_access(Access& acc) { acc.child<T>().write(); }
    static void prepare(Blackboard& bb) { bb.get_or_emplace<T>(); }
    static T& fetch(Blackboard& bb) { return bb.get<T>(); }
};

// ==================== 黑板本身 ====================

template <>
struct ParamProvider<Blackboard> {
    static void build_access(Access&) { /* 黑板本身不需要访问控制 */ }
    static void prepare(Blackboard&) { /* 无需准备 */ }
    static Blackboard& fetch(Blackboard& bb) { return bb; }
};

// ==================== 实体操作 ====================

template <>
struct ParamProvider<EntityQuery> {
    static void build_access(Access& acc) { acc.child<Registry>().read(); }
    static void prepare(Blackboard& bb) { bb.get_or_emplace<Registry>(); }
    static EntityQuery fetch(Blackboard& bb) { return EntityQuery(bb.get<Registry>()); }
};

template <>
struct ParamProvider<EntityCreator> {
    static void build_access(Access& acc) { acc.child<Registry>().write(); }
    static void prepare(Blackboard& bb) { bb.get_or_emplace<Registry>(); }
    static EntityCreator fetch(Blackboard& bb) { return EntityCreator(bb.get<Registry>()); }
};

template <>
struct ParamProvider<EntityDestroyer> {
    static void build_access(Access& acc) {
        acc.child<Registry>().write();
        acc.child<Entity>().write();
    }
    static void prepare(Blackboard& bb) { bb.get_or_emplace<Registry>(); }
    static EntityDestroyer fetch(Blackboard& bb) { return EntityDestroyer(bb.get<Registry>()); }
};

template <>
struct ParamProvider<EntityCommandBuffer> {
    static void build_access(Access& acc) {
        // 命令缓冲区无冲突，每个 Pass 独立拥有
    }
    static void prepare(Blackboard& bb) {
        // 每个 Pass 会通过 fetch 获取自己的实例
    }
    static EntityCommandBuffer& fetch(Blackboard& bb) {
        return bb.get_or_emplace<EntityCommandBuffer>();
    }
};

// ==================== 组件读写 ====================

template <typename... Cs>
struct ParamProvider<ComponentReader<Cs...>> {
    static void build_access(Access& acc) {
        auto& reg_acc = acc.child<Registry>().read();
        (reg_acc.template child<Cs>().read(), ...);
    }
    static void prepare(Blackboard& bb) {
        auto& reg = bb.get_or_emplace<Registry>();
        (reg.template storage<std::remove_const_t<Cs>>(), ...);
        (reg.template storage<AddTag<Cs>>(), ...);
        (reg.template storage<ChangeTag<Cs>>(), ...);
        (reg.template storage<RemoveTag<Cs>>(), ...);
        (reg.template storage<AddDelayed<Cs>>(), ...);
        (reg.template storage<ChangeDelayed<Cs>>(), ...);
        (reg.template storage<RemoveDelayed<Cs>>(), ...);
    }
    static ComponentReader<Cs...> fetch(Blackboard& bb) {
        return ComponentReader<Cs...>(bb.get<Registry>());
    }
};

template <typename... Cs>
struct ParamProvider<ComponentWriter<Cs...>> {
    static void build_access(Access& acc) {
        auto& reg_acc = acc.child<Registry>().read();
        (reg_acc.template child<Cs>().write(), ...);
    }
    static void prepare(Blackboard& bb) {
        auto& reg = bb.get_or_emplace<Registry>();
        (reg.template storage<std::remove_const_t<Cs>>(), ...);
        (reg.template storage<AddTag<Cs>>(), ...);
        (reg.template storage<ChangeTag<Cs>>(), ...);
        (reg.template storage<RemoveTag<Cs>>(), ...);
        (reg.template storage<AddDelayed<Cs>>(), ...);
        (reg.template storage<ChangeDelayed<Cs>>(), ...);
        (reg.template storage<RemoveDelayed<Cs>>(), ...);
    }
    static ComponentWriter<Cs...> fetch(Blackboard& bb) {
        return ComponentWriter<Cs...>(bb.get<Registry>());
    }
};

// ==================== 事件双缓冲 ====================

template <typename T>
struct ParamProvider<EventReader<T>> {
    static void build_access(Access& acc) {
        acc.template child<DoubleBuffer<T>>().read();
    }
    static void prepare(Blackboard& bb) {
        if (!bb.has<DoubleBuffer<T>>()) {
            bb.emplace<DoubleBuffer<T>>();
            bb.add_cleanup([&bb]() { bb.get<DoubleBuffer<T>>().swap(); });
        }
    }
    static EventReader<T> fetch(Blackboard& bb) {
        return EventReader<T>(bb.get<DoubleBuffer<T>>().read_buf);
    }
};

template <typename T>
struct ParamProvider<EventWriter<T>> {
    static void build_access(Access& acc) {
        acc.template child<DoubleBuffer<T>>().write();
    }
    static void prepare(Blackboard& bb) {
        if (!bb.has<DoubleBuffer<T>>()) {
            bb.emplace<DoubleBuffer<T>>();
            bb.add_cleanup([&bb]() { bb.get<DoubleBuffer<T>>().swap(); });
        }
    }
    static EventWriter<T> fetch(Blackboard& bb) {
        return EventWriter<T>(bb.get<DoubleBuffer<T>>().write_buf);
    }
};

// ==================== 上下文访问 ====================

template <typename T>
struct ParamProvider<ContextReader<T>> {
    static void build_access(Access& acc) {
        acc.template child<ContextStorage<T>>().read();
    }
    static void prepare(Blackboard& bb) {
        bb.get_or_emplace<ContextStorage<T>>();
    }
    static ContextReader<T> fetch(Blackboard& bb) {
        return ContextReader<T>(bb.get<ContextStorage<T>>().data);
    }
};

template <typename T>
struct ParamProvider<ContextWriter<T>> {
    static void build_access(Access& acc) {
        acc.template child<ContextStorage<T>>().write();
    }
    static void prepare(Blackboard& bb) {
        bb.get_or_emplace<ContextStorage<T>>();
    }
    static ContextWriter<T> fetch(Blackboard& bb) {
        return ContextWriter<T>(bb.get<ContextStorage<T>>().data);
    }
};

} // namespace fe::engine
