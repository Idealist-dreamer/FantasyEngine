#pragma once

namespace fe::engine {

extern volatile int _force_link_mimalloc_v;
}

static int _linker_helper = (int)fe::engine::_force_link_mimalloc_v;