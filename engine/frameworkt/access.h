#pragma once

#include "common.h"

namespace fe::engine {

// 访问模式
enum class AccessMode : uint8_t { None = 0, Read = 1, Write = 2 };

// 访问树节点：表示对某个类型资源的访问权限，支持父子关系（如 Registry -> Component）
class Access {
public:
    explicit Access(std::type_index t = typeid(void)) : m_type(t) {}

    // 构建读访问
    Access& read() {
        if (m_mode == AccessMode::None) m_mode = AccessMode::Read;
        return *this;
    }

    // 构建写访问
    Access& write() {
        m_mode = AccessMode::Write;
        return *this;
    }

    // 获取或创建子节点
    template <typename T>
    Access& child() {
        std::type_index id(typeid(T));
        auto& child_ptr = m_children[id];
        if (!child_ptr)
            child_ptr = stl::make_unique<Access>(id);
        return *child_ptr;
    }

    // 合并另一棵访问树（用于累积多个参数的访问需求）
    void merge(const Access& other) {
        // 模式合并：写优先于读，读优先于无
        if (other.m_mode == AccessMode::Write)
            m_mode = AccessMode::Write;
        else if (other.m_mode == AccessMode::Read && m_mode == AccessMode::None)
            m_mode = AccessMode::Read;

        // 递归合并子节点
        for (const auto& [id, other_child] : other.m_children) {
            auto& my_child = m_children[id];
            if (!my_child)
                my_child = stl::make_unique<Access>(id);
            my_child->merge(*other_child);
        }
    }

    // 烘焙：计算子树的最大模式，用于快速冲突检测
    void bake() {
        m_tree_max = m_mode;
        for (const auto& [_, child] : m_children) {
            child->bake();
            if (child->m_tree_max == AccessMode::Write)
                m_tree_max = AccessMode::Write;
            else if (child->m_tree_max == AccessMode::Read && m_tree_max == AccessMode::None)
                m_tree_max = AccessMode::Read;
        }
    }

    // 冲突检测：与另一棵访问树比较
    bool is_conflict(const Access& other) const {
        if (m_type != other.m_type) return false;

        // 先检查整树最大模式冲突
        if (is_mode_conflict(m_tree_max, other.m_tree_max)) return true;

        // 检查当前节点模式与对方树最大模式
        if (is_mode_conflict(m_mode, other.m_tree_max)) return true;
        if (is_mode_conflict(other.m_mode, m_tree_max)) return true;

        // 递归检查子节点
        for (const auto& [id, my_child] : m_children) {
            auto it = other.m_children.find(id);
            if (it != other.m_children.end()) {
                if (my_child->is_conflict(*(it->second)))
                    return true;
            }
        }
        return false;
    }

private:
    static bool is_mode_conflict(AccessMode a, AccessMode b) {
        if (a == AccessMode::None || b == AccessMode::None) return false;
        return a == AccessMode::Write || b == AccessMode::Write;
    }

    std::type_index m_type;
    AccessMode m_mode = AccessMode::None;
    AccessMode m_tree_max = AccessMode::None;
    stl::unordered_map<std::type_index, stl::unique_ptr<Access>> m_children;
};

} // namespace fe::engine
