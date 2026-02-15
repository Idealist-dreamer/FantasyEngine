#pragma once

namespace fe::engine::memory {
extern volatile int _force_link_mimalloc_v;
}

static int _linker_helper = (int)fe::engine::memory::_force_link_mimalloc_v;