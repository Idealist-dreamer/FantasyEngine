#ifdef FE_OVER_NEW_DEL_MIMALLOC
#define MI_MALLOC_OVERRIDE
#include <mimalloc.h>
#include <mimalloc-new-delete.h>
#endif

namespace fe::engine {

volatile int _force_link_mimalloc_v = 0;
}  // namespace fe::engine