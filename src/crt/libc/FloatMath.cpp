// openTIT - single precision maths functions of the C runtime (libm, float variants).
//
// The console library implements these with polynomial/rational kernels; on
// PC the host <cmath> provides the same functions.
#include "crt/libc/Libc.h"

#include <cmath>
#include <cstdint>
#include <cstring>

namespace crt {

// @func 0x005E693C SqrtF
float SqrtF(float x)
{
    return std::sqrt(x);
}

// @func 0x005E6440 FModF
// Remainder of x / |y| with the sign of x (fmodf).
float FModF(float x, float y)
{
    return std::fmod(x, y);
}

// @func 0x005E62E8 TanF
float TanF(float x)
{
    return std::tan(x);
}

// @func 0x005E6578 ASinF
// Arguments outside [-1, 1] yield NaN.
float ASinF(float x)
{
    return std::asin(x);
}

// @func 0x005E649C ACosF
float ACosF(float x)
{
    return std::acos(x);
}

// @func 0x005E67D4 ATanF
float ATanF(float x)
{
    return std::atan(x);
}

// @func 0x005E665C ATan2F
float ATan2F(float y, float x)
{
    return std::atan2(y, x);
}

// @func 0x005E6830 ATanKernelF
// Shared arc-tangent kernel for x in [0, 1]. Callers pass an octant code:
// bit 1 = complement, bit 2 = upper half, bit 3 = negate. Arguments above
// tan(pi/12) are reduced by pi/6 internally (bit 0). Bits 0-2 then select an
// offset from {0, pi/6, pi/2, pi/3, pi/2, 2pi/3, pi, 5pi/6}.
float ATanKernelF(float x, uint32_t octant)
{
    static const float kOffsets[8] = {
        0.0f, 0.52359879f, 1.5707964f, 1.0471976f, 1.5707964f, 2.0943952f, 3.1415927f, 2.6179938f };
    float r = std::atan(x);
    if (x > 0.26794919f) {
        octant |= 1;
        r -= 0.52359879f;  // the reduction step; the offset table adds pi/6 back
    }
    if (octant & 2)
        r = -r;
    r += kOffsets[octant & 7];
    return (octant & 8) ? -r : r;
}

// @func 0x005E68D0 FloatClassify
// Returns 2 for NaN, 1 for infinity, 0 for zero and -1 for finite non-zero values.
int FloatClassify(const float* value)
{
    uint32_t bits;
    std::memcpy(&bits, value, sizeof(bits));
    const uint32_t exponent = (bits >> 23) & 0xFF;
    const uint32_t mantissa = bits & 0x7FFFFF;
    if (exponent == 0xFF)
        return mantissa ? 2 : 1;
    if (exponent == 0 && mantissa == 0)
        return 0;
    return -1;
}

} // namespace crt
