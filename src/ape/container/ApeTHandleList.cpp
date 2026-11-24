// openTIT - ApeTHandleList<T> support and instantiations.
#include "ape/container/ApeTHandleList.h"

namespace ape {

namespace {
uint32_t g_handleListSerial = 0;   // 0x010C29DC
}

uint32_t NextHandleListSerial()
{
    if (++g_handleListSerial > 0xFFFF)
        g_handleListSerial = 1;
    return g_handleListSerial;
}

// One instantiation per element type on the console; Init and Grow compiled
// to identical code for all of them.
// @func 0x0079BA10 ApeTHandleList<T>::Init (copy 1)
// @func 0x0079B0F0 ApeTHandleList<T>::Init (copy 2)
// @func 0x0079B250 ApeTHandleList<T>::Init (copy 3)
// @func 0x0079A1C8 ApeTHandleList<T>::Init (copy 4)
// @func 0x00784C84 ApeTHandleList<T>::Init (copy 5)
// @func 0x0079BB70 ApeTHandleList<T>::Grow (copy 1)
// @func 0x0079B3B0 ApeTHandleList<T>::Grow (copy 2)
// @func 0x0079A528 ApeTHandleList<T>::Grow (copy 3)
// @func 0x00784FE4 ApeTHandleList<T>::Grow (copy 4)
// @func 0x0079BD1C ApeTHandleList<int32_t>::PushBack (trophy unlock queue)
// @func 0x0079C07C ApeTHandleList<int32_t>::PopFront (trophy unlock queue)
template class ApeTHandleList<int32_t>;

// @func 0x0079B55C ApeTHandleList<void*>::PushBack
// Two further pointer-sized instantiations (the spawner's list and one in
// the engine at 0x004F480C) compiled PushBack with the handle returned
// through memory; the logic is the same.
// @func 0x00785190 ApeTHandleList<void*>::PushBack (copy 2, spawner)
// @func 0x0079A6D4 ApeTHandleList<void*>::PushBack (copy 3)
template class ApeTHandleList<void*>;

} // namespace ape
