#pragma once

#include "common.h"

namespace fe::engine {

// 黑板：类型擦除的键值存储，支持任意类型的存取
class Blackboard {
public:
    Blackboard() = default;
    ~Blackboard() = default;
    Blackboard(const Blackboard&) = delete;
    Blackboard& operator=(const Blackboard&) = delete;

    // 查询是否存在某类型
    template <typename T>
    bool has() const {
        return m_data.find(std::type_index(typeid(T))) != m_data.end();
    }

    // 放置新对象，返回引用
    template <typename T, typename... Args>
    T& emplace(Args&&... args) {
        auto result = m_data.emplace(std::type_index(typeid(T)),
                                     Any::create<T>(std::forward<Args>(args)...));
        return *result.first->second.template get<T>();
    }

    // 获取对象引用（必须存在）
    template <typename T>
    T& get() {
        return *m_data.at(std::type_index(typeid(T))).template get<T>();
    }
    template <typename T>
    const T& get() const {
        return *m_data.at(std::type_index(typeid(T))).template get<T>();
    }

    // 获取或创建对象
    template <typename T, typename... Args>
    T& get_or_emplace(Args&&... args) {
        if (auto it = m_data.find(std::type_index(typeid(T))); it != m_data.end())
            return *it->second.template get<T>();
        return emplace<T>(std::forward<Args>(args)...);
    }

    // 添加帧末清理函数（每帧执行一次，之后自动清除）
    void add_cleanup(std::function<void()> func) {
        m_cleanups.push_back(std::move(func));
    }

    // 执行所有清理函数
    void execute_cleanups() {
        for (auto& f : m_cleanups) f();
        m_cleanups.clear();
    }

private:
    stl::unordered_map<std::type_index, Any> m_data;
    stl::vector<std::function<void()>> m_cleanups;
};

} // namespace fe::engine
