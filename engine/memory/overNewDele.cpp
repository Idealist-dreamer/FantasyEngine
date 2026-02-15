#define MI_MALLOC_OVERRIDE

#include <mimalloc.h>

#include <mimalloc-new-delete.h>

namespace fe::engine {
int _force_link_mimalloc_v = 0;
}