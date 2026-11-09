#ifndef LIBX87_NATIVE_INCLUDED
#define LIBX87_NATIVE_INCLUDED

// Host x87 evaluation when enabled on an arithmetic instance, used for
// 64-bit precision with nearest rounding while the executing FPU uses the
// same control. X87_Float80 stores the significand at offset 0 and the
// sign/exponent at offset 8, the x87 memory format, so values move with
// fld/fstp tbyte.
//
// Native arithmetic covers the operations every x87 implementation
// defines identically: load, store, add, subtract, multiply, divide, square
// root, round to integer and scale. native transcendentals
// covers fsin, fcos, fpatan, f2xm1 and fyl2x, whose last bit differs between
// processor vendors; results then follow the executing processor, as the
// original build's do.
//
// 32-bit MSVC uses __asm, which it does not inline; GCC and Clang (-m32) use
// long double.

#if !(defined(_M_IX86) || defined(__i386__))
#error "HARD x87 selections require a 32-bit x86 target"
#endif

#include <string.h>

// A host callback or nested precision scope can change this after an
// arithmetic object is constructed. Check the executing FPU for every use.
inline bool x87_native_control_matches()
{
    unsigned short control;
#if defined(_MSC_VER)
    __asm fnstcw control
#else
    __asm__ __volatile__("fnstcw %0" : "=m"(control));
#endif
    return (control & 0x0f00) == 0x0300;
}

#if defined(_MSC_VER)

static X87_Float80 x87_native_load_float(float value)
{
    X87_Float80 result;
    __asm fld value
    __asm fstp tbyte ptr result
    return result;
}

static X87_Float80 x87_native_load_double(double value)
{
    X87_Float80 result;
    __asm fld value
    __asm fstp tbyte ptr result
    return result;
}

static float x87_native_store_float(const X87_Float80 &value)
{
    float result;
    const X87_Float80 *source = &value;
    __asm mov eax, source
    __asm fld tbyte ptr [eax]
    __asm fstp result
    return result;
}

static double x87_native_store_double(const X87_Float80 &value)
{
    double result;
    const X87_Float80 *source = &value;
    __asm mov eax, source
    __asm fld tbyte ptr [eax]
    __asm fstp result
    return result;
}

// Loads first then second (ST1 = first, ST0 = second) for the operation.
#define X87_NATIVE_LOAD_PAIR(FIRST, SECOND)                                                                     \
    const X87_Float80 *first_source = &(FIRST), *second_source = &(SECOND);                                        \
    __asm { mov eax, first_source }                                                                                  \
    __asm { fld tbyte ptr [eax] }                                                                                    \
    __asm { mov eax, second_source }                                                                                 \
    __asm { fld tbyte ptr [eax] }

static X87_Float80 x87_native_add(const X87_Float80 &first, const X87_Float80 &second)
{
    X87_Float80 result;
    X87_NATIVE_LOAD_PAIR(first, second)
    __asm faddp st(1), st
    __asm fstp tbyte ptr result
    return result;
}

static X87_Float80 x87_native_subtract(const X87_Float80 &first, const X87_Float80 &second)
{
    X87_Float80 result;
    X87_NATIVE_LOAD_PAIR(first, second)
    __asm fsubp st(1), st
    __asm fstp tbyte ptr result
    return result;
}

static X87_Float80 x87_native_multiply(const X87_Float80 &first, const X87_Float80 &second)
{
    X87_Float80 result;
    X87_NATIVE_LOAD_PAIR(first, second)
    __asm fmulp st(1), st
    __asm fstp tbyte ptr result
    return result;
}

static X87_Float80 x87_native_divide(const X87_Float80 &first, const X87_Float80 &second)
{
    X87_Float80 result;
    X87_NATIVE_LOAD_PAIR(first, second)
    __asm fdivp st(1), st
    __asm fstp tbyte ptr result
    return result;
}

// atan(y / x)
static X87_Float80 x87_native_arctangent(const X87_Float80 &y, const X87_Float80 &x)
{
    X87_Float80 result;
    X87_NATIVE_LOAD_PAIR(y, x)
    __asm fpatan
    __asm fstp tbyte ptr result
    return result;
}

// y * log2(x)
static X87_Float80 x87_native_log2_multiply(const X87_Float80 &x, const X87_Float80 &y)
{
    X87_Float80 result;
    X87_NATIVE_LOAD_PAIR(y, x)
    __asm fyl2x
    __asm fstp tbyte ptr result
    return result;
}

static X87_Float80 x87_native_scale(const X87_Float80 &value, const X87_Float80 &exponent)
{
    X87_Float80 result;
    X87_NATIVE_LOAD_PAIR(exponent, value)
    __asm fscale
    __asm fstp tbyte ptr result
    __asm fstp st(0)
    return result;
}

#define X87_NATIVE_UNARY(NAME, INSTRUCTION)                                                                      \
    static X87_Float80 NAME(const X87_Float80 &value)                                                            \
    {                                                                                                                \
        X87_Float80 result;                                                                                        \
        const X87_Float80 *source = &value;                                                                        \
        __asm { mov eax, source }                                                                                    \
        __asm { fld tbyte ptr [eax] }                                                                                \
        __asm { INSTRUCTION }                                                                                        \
        __asm { fstp tbyte ptr result }                                                                              \
        return result;                                                                                               \
    }

#else

inline long double x87_native_in(const X87_Float80 &value)
{
    long double result = 0;
    memcpy(&result, &value, 10);
    return result;
}

inline X87_Float80 x87_native_out(long double value)
{
    X87_Float80 result;
    memcpy(&result, &value, 10);
    return result;
}

inline X87_Float80 x87_native_load_float(float value) { return x87_native_out(value); }
inline X87_Float80 x87_native_load_double(double value) { return x87_native_out(value); }

inline float x87_native_store_float(const X87_Float80 &value)
{
    volatile float result = static_cast<float>(x87_native_in(value)); // one fstp: no double rounding
    return result;
}

inline double x87_native_store_double(const X87_Float80 &value)
{
    volatile double result = static_cast<double>(x87_native_in(value));
    return result;
}

inline X87_Float80 x87_native_add(const X87_Float80 &first, const X87_Float80 &second)
{
    return x87_native_out(x87_native_in(first) + x87_native_in(second));
}

inline X87_Float80 x87_native_subtract(const X87_Float80 &first, const X87_Float80 &second)
{
    return x87_native_out(x87_native_in(first) - x87_native_in(second));
}

inline X87_Float80 x87_native_multiply(const X87_Float80 &first, const X87_Float80 &second)
{
    return x87_native_out(x87_native_in(first) * x87_native_in(second));
}

inline X87_Float80 x87_native_divide(const X87_Float80 &first, const X87_Float80 &second)
{
    return x87_native_out(x87_native_in(first) / x87_native_in(second));
}

// atan(y / x)
inline X87_Float80 x87_native_arctangent(const X87_Float80 &y, const X87_Float80 &x)
{
    long double result;
    __asm__("fpatan" : "=t"(result) : "0"(x87_native_in(x)), "u"(x87_native_in(y)) : "st(1)");
    return x87_native_out(result);
}

// y * log2(x)
inline X87_Float80 x87_native_log2_multiply(const X87_Float80 &x, const X87_Float80 &y)
{
    long double result;
    __asm__("fyl2x" : "=t"(result) : "0"(x87_native_in(x)), "u"(x87_native_in(y)) : "st(1)");
    return x87_native_out(result);
}

inline X87_Float80 x87_native_scale(const X87_Float80 &value, const X87_Float80 &exponent)
{
    long double result;
    __asm__("fscale" : "=t"(result) : "0"(x87_native_in(value)), "u"(x87_native_in(exponent)));
    return x87_native_out(result);
}

#define X87_NATIVE_UNARY(NAME, INSTRUCTION)                                                                      \
    inline X87_Float80 NAME(const X87_Float80 &value)                                                            \
    {                                                                                                                \
        long double operand = x87_native_in(value);                                                              \
        __asm__(#INSTRUCTION : "+t"(operand));                                                                       \
        return x87_native_out(operand);                                                                          \
    }

#endif

X87_NATIVE_UNARY(x87_native_square_root, fsqrt)
X87_NATIVE_UNARY(x87_native_round_integer, frndint)
X87_NATIVE_UNARY(x87_native_sine, fsin)
X87_NATIVE_UNARY(x87_native_cosine, fcos)
X87_NATIVE_UNARY(x87_native_power2_minus1, f2xm1)

#undef X87_NATIVE_UNARY
#undef X87_NATIVE_LOAD_PAIR

#endif
