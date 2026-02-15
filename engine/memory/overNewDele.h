#pragma once

namespace fe::engine {
extern int _force_link_mimalloc_v;
}  // namespace fe::engine

static int _linker_helper = fe::engine::_force_link_mimalloc_v;