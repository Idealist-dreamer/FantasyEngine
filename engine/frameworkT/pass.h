#pragma once

#include "paramBinder.h"
#include "stage.h"
#include "foundation/utility/meta.h"
#include <functional>

namespace fe::engine {

// Pass: Execution unit with auto-derived access tree
class Pass {
public:
    Pass(stl::string name, bool repeat = true, uint32_t priority = Priority::Low)
        : m_name(std::move(name)), m_repeat(repeat), m_priority(priority) {}

    // Create startup pass (runs once)
    template<typename StageT, typename Func>
    static Pass create_start(stl::string name, Func&& func, uint32_t priority = Priority::First) {
        Pass pass(std::move(name), false, priority);
        pass.m_stage = stage::get_stage_hash<StageT>();
        pass.bind(std::forward<Func>(func));
        return pass;
    }

    // Create update pass (runs every frame)
    template<typename StageT, typename Func>
    static Pass create_update(stl::string name, Func&& func, uint32_t priority = Priority::Low) {
        Pass pass(std::move(name), true, priority);
        pass.m_stage = stage::get_stage_hash<StageT>();
        pass.bind(std::forward<Func>(func));
        return pass;
    }

    // Bind function with auto access derivation
    template<typename Func>
    void bind(Func&& func) {
        using ArgsTuple = typename meta::function_traits<std::remove_cvref_t<Func>>::args_tuple;
        bind_impl(std::forward<Func>(func), static_cast<ArgsTuple*>(nullptr));
    }

    // Set stage
    template<typename StageT>
    Pass& set_stage() {
        m_stage = stage::get_stage_hash<StageT>();
        return *this;
    }

    // Access
    bool is_conflict(const Pass& other) const { return m_access.is_conflict(other.m_access); }
    
    void prepare(Blackboard& bb) const { if (m_prepare) m_prepare(bb); }
    void execute(Blackboard& bb) const { if (m_execute) m_execute(bb); }

    // Public data
    stl::string m_name;
    bool m_repeat = true;
    uint32_t m_priority = Priority::Low;
    stage::StageHash m_stage = 0;
    Access m_access;

private:
    template<typename Func, typename... Args>
    void bind_impl(Func&& func, std::tuple<Args...>*) {
        // Derive access tree from parameter types
        m_access = ParamOps::build_access<Args...>();

        // Create prepare function
        m_prepare = [](Blackboard& bb) {
            ParamOps::prepare<Args...>(bb);
        };

        // Create type-erased execute function
        m_execute = [f = std::forward<Func>(func)](Blackboard& bb) mutable {
            auto params = ParamOps::fetch<Args...>(bb);
            std::apply(f, std::move(params));
        };
    }

    std::function<void(Blackboard&)> m_prepare;
    std::function<void(Blackboard&)> m_execute;
};

} // namespace fe::engine
