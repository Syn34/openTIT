// openTIT - ApeTFixedArray<T> instantiations (see ApeTFixedArray.h).
#include "ape/container/ApeTFixedArray.h"

namespace ape {

// 4-byte elements (the pools of the world module's lists use this shape).
template class ApeTFixedArray<uint32_t>;

} // namespace ape
