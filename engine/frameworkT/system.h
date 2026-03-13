#pragma once

#include "pass.h"

namespace fe::engine {

// Forward declaration
class Archive;

// System: Base class for user-defined systems
class System {
public:
    explicit System(stl::string name) : m_name(std::move(name)) {}
    virtual ~System() = default;

    // Initialize system, return false to skip
    virtual bool init(Blackboard& bb) = 0;

    // Serialization hooks
    virtual void save(Blackboard& bb, Archive& ar) {}
    virtual void load(Blackboard& bb, Archive& ar) {}

protected:
    stl::string m_name;
    stl::vector<Pass> m_passes;

    friend class Scene;
};

// Helper macro for adding passes
#define FE_SYS_PASS(ClassName, FuncName, Stage) \
    m_passes.push_back(Pass::create_update<Stage>(#FuncName, [this](auto&&... args) { \
        this->FuncName(std::forward<decltype(args)>(args)...); \
    }))

} // namespace fe::engine
