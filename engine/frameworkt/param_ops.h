#pragma once

#include "param_provider.h"

namespace fe::engine {

struct ParamOps {
    // 从多个参数构建合并后的访问树
    template <typename... Args>
    static Access build_access() {
        Access root;
        (ParamProvider<clean_t<Args>>::build_access(root), ...);
        root.bake();
        return root;
    }

    // 准备所有参数所需的数据
    template <typename... Args>
    static void prepare(Blackboard& bb) {
        (ParamProvider<clean_t<Args>>::prepare(bb), ...);
    }

    // 获取所有参数实例（打包为元组，保持引用语义）
    template <typename... Args>
    static auto fetch(Blackboard& bb) {
        return std::tuple<decltype(ParamProvider<clean_t<Args>>::fetch(bb))...>(
            ParamProvider<clean_t<Args>>::fetch(bb)...
        );
    }
};

} // namespace fe::engine
