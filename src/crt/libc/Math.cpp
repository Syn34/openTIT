// openTIT - C runtime <math.h> internals.
//
// Dinkumware's math library is built on a few helpers that take a float or
// double apart and put it back together by poking at its bit pattern. They
// report what kind of value they found with a small classification code.
// The host's ldexp/frexp do the same arithmetic.
#include "crt/libc/Libc.h"

#include <cmath>

namespace crt {

namespace {

template <typename T>
short ScaleValue(T* value, long exponent)
{
    switch (std::fpclassify(*value)) {
    case FP_NAN:
        return kFloat_NaN;
    case FP_INFINITE:
        return kFloat_Inf;
    case FP_ZERO:
        return kFloat_Zero;
    default:
        break;
    }
    // Clamp so the int conversion cannot wrap; anything this large has
    // overflowed or underflowed completely anyway.
    const long limit = 100000;
    const int e = (int)(exponent > limit ? limit : (exponent < -limit ? -limit : exponent));
    *value = std::ldexp(*value, e);
    if (std::isinf(*value))
        return kFloat_Inf;
    if (*value == 0)
        return kFloat_Zero;
    return kFloat_Finite;
}

} // namespace

// @func 0x0071F490 FloatScale
// _FDscale(): multiplies *value by 2^exponent in place. Overflow gives a
// signed infinity, total underflow a signed zero. Returns the class of the
// original value for NaN/infinity/zero, otherwise the class of the result.
// NOTE: the original truncates when the result becomes subnormal; ldexp rounds.
short FloatScale(float* value, long exponent)
{
    return ScaleValue(value, exponent);
}

// @func 0x0072AD98 DoubleScale
// _Dscale(): the double precision version of FloatScale.
short DoubleScale(double* value, long exponent)
{
    return ScaleValue(value, exponent);
}

// @func 0x0072DB48 LongDoubleScale
// _LDscale(): long double is the same type as double on the console, so this
// simply forwards to DoubleScale.
short LongDoubleScale(double* value, long exponent)
{
    return DoubleScale(value, exponent);
}

// @func 0x0071F668 FloatUnscale
// _FDunscale(): splits a finite non-zero *value into a fraction with
// magnitude in [0.5, 1) (left in *value, sign kept) and a power of two
// (stored in *exponent), returning kFloat_Finite. Zero, infinity and NaN are
// left alone with *exponent = 0, returning their class.
short FloatUnscale(short* exponent, float* value)
{
    switch (std::fpclassify(*value)) {
    case FP_NAN:
        *exponent = 0;
        return kFloat_NaN;
    case FP_INFINITE:
        *exponent = 0;
        return kFloat_Inf;
    case FP_ZERO:
        *exponent = 0;
        return kFloat_Zero;
    default:
        break;
    }
    int e = 0;
    *value = std::frexp(*value, &e);
    *exponent = (short)e;
    return kFloat_Finite;
}

} // namespace crt
