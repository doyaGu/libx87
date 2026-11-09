#ifndef LIBX87_INCLUDED
#define LIBX87_INCLUDED

#include <stdint.h>

// Keep the library ABI when included by a host with different packing.
#pragma pack(push, 8)

class X87_Arithmetic;

// An extended value has no dependency on the host floating-point ABI.
class X87_Float80
{
    uint64_t significand;
    uint16_t sign_exponent;

public:
    X87_Float80() : significand(0), sign_exponent(0) {}
    static X87_Float80 from_bits(uint16_t exponent, uint64_t fraction)
    {
        X87_Float80 result;
        result.sign_exponent = exponent;
        result.significand = fraction;
        return result;
    }
    static X87_Float80 from_bytes(const unsigned char *bytes);
    void to_bytes(unsigned char *bytes) const;
    uint64_t get_significand() const { return significand; }
    uint16_t get_sign_exponent() const { return sign_exponent; }
};

#include "detail/fast.hxx"
#if defined(_M_IX86) || defined(__i386__)
#include "detail/native.hxx"
#endif

// Each numerical kernel has explicit precision, rounding and exception status.
// Its storage is fixed; constructing or operating on it never allocates.
class X87_Arithmetic
{
    union StatusStorage
    {
        uint64_t alignment;
        unsigned char bytes[16];
    } status_storage;
    // 64-bit precision with nearest-even rounding: normal cases of the
    // inline operations use fast.hxx. Their inexact results are
    // merged into the exception flags.
    bool fast_path;
    bool fast_inexact;
    bool native_arithmetic;
    bool native_transcendental;

    X87_Arithmetic(const X87_Arithmetic &);
    X87_Arithmetic &operator=(const X87_Arithmetic &);
    void *get_status() { return status_storage.bytes; }
    const void *get_status() const { return status_storage.bytes; }
    void update_fast_path();
    bool use_native_path() const
    {
#if defined(_M_IX86) || defined(__i386__)
        return fast_path && x87_native_control_matches();
#else
        return false;
#endif
    }

    X87_Float80 load_general(float value);
    X87_Float80 load_general(double value);
    float store_float_general(const X87_Float80 &value);
    double store_double_general(const X87_Float80 &value);
    X87_Float80 add_general(const X87_Float80 &first, const X87_Float80 &second);
    X87_Float80 subtract_general(const X87_Float80 &first, const X87_Float80 &second);
    X87_Float80 multiply_general(const X87_Float80 &first, const X87_Float80 &second);
    X87_Float80 divide_general(const X87_Float80 &first, const X87_Float80 &second);

public:
    enum Precision { PRECISION_24 = 24, PRECISION_53 = 53, PRECISION_64 = 64 };
    enum Rounding { ROUND_NEAREST = 0, ROUND_DOWN = 1, ROUND_UP = 2, ROUND_TO_ZERO = 3 };
    enum Relation { LESS = -1, EQUAL = 0, GREATER = 1, UNORDERED = 2 };
    enum Exception
    {
        EXCEPTION_INVALID_OPERATION = 0x01,
        EXCEPTION_DENORMAL_INPUT = 0x02,
        EXCEPTION_DIVISION_BY_ZERO = 0x04,
        EXCEPTION_OVERFLOW = 0x08,
        EXCEPTION_UNDERFLOW = 0x10,
        EXCEPTION_INEXACT_RESULT = 0x20
    };

    explicit X87_Arithmetic(Precision precision = PRECISION_64,
        Rounding rounding = ROUND_NEAREST, bool native_arithmetic = false,
        bool native_transcendental = false);
    void set_precision(Precision precision);
    void set_rounding(Rounding rounding);
    unsigned get_exception_flags() const;
    void clear_exception_flags();
    void raise_exception_flags(unsigned flags);

    X87_Float80 load(float value)
    {
#if defined(_M_IX86) || defined(__i386__)
        if(native_arithmetic && use_native_path()) return x87_native_load_float(value);
#endif
        X87_Float80 result;
        if(fast_path && x87_fast_load_float(value, &result)) return result;
        return load_general(value);
    }
    X87_Float80 load(double value)
    {
#if defined(_M_IX86) || defined(__i386__)
        if(native_arithmetic && use_native_path()) return x87_native_load_double(value);
#endif
        X87_Float80 result;
        if(fast_path && x87_fast_load_double(value, &result)) return result;
        return load_general(value);
    }
    X87_Float80 load(int32_t value);
    float store_float(const X87_Float80 &value)
    {
#if defined(_M_IX86) || defined(__i386__)
        if(native_arithmetic && use_native_path()) return x87_native_store_float(value);
#endif
        float result;
        if(fast_path && x87_fast_store_float(value, &result, &fast_inexact)) return result;
        return store_float_general(value);
    }
    double store_double(const X87_Float80 &value)
    {
#if defined(_M_IX86) || defined(__i386__)
        if(native_arithmetic && use_native_path()) return x87_native_store_double(value);
#endif
        double result;
        if(fast_path && x87_fast_store_double(value, &result, &fast_inexact)) return result;
        return store_double_general(value);
    }
    // Scalar API returns use the host ABI. On a native x87 ABI this retains
    // the extended result; internal portable call chains use X87_Float80.
    float return_float(const X87_Float80 &value);
    double return_double(const X87_Float80 &value);

    X87_Float80 add(const X87_Float80 &first, const X87_Float80 &second)
    {
#if defined(_M_IX86) || defined(__i386__)
        if(native_arithmetic && use_native_path()) return x87_native_add(first, second);
#endif
        X87_Float80 result;
        if(fast_path && x87_fast_add(first, second, 0, &result, &fast_inexact)) return result;
        return add_general(first, second);
    }
    X87_Float80 subtract(const X87_Float80 &first, const X87_Float80 &second)
    {
#if defined(_M_IX86) || defined(__i386__)
        if(native_arithmetic && use_native_path()) return x87_native_subtract(first, second);
#endif
        X87_Float80 result;
        if(fast_path && x87_fast_add(first, second, 1, &result, &fast_inexact)) return result;
        return subtract_general(first, second);
    }
    X87_Float80 multiply(const X87_Float80 &first, const X87_Float80 &second)
    {
#if defined(_M_IX86) || defined(__i386__)
        if(native_arithmetic && use_native_path()) return x87_native_multiply(first, second);
#endif
        X87_Float80 result;
        if(fast_path && x87_fast_multiply(first, second, &result, &fast_inexact)) return result;
        return multiply_general(first, second);
    }
    X87_Float80 divide(const X87_Float80 &first, const X87_Float80 &second)
    {
#if defined(_M_IX86) || defined(__i386__)
        if(native_arithmetic && use_native_path()) return x87_native_divide(first, second);
#endif
        X87_Float80 result;
        if(fast_path && x87_fast_divide(first, second, &result, &fast_inexact)) return result;
        return divide_general(first, second);
    }
    X87_Float80 square_root(const X87_Float80 &value);
    X87_Float80 absolute(const X87_Float80 &value);
    X87_Float80 negate(const X87_Float80 &value);
    Relation compare(const X87_Float80 &first, const X87_Float80 &second,
        bool quiet = false);

    X87_Float80 sine(const X87_Float80 &value, bool *out_of_range = 0);
    X87_Float80 cosine(const X87_Float80 &value, bool *out_of_range = 0);
    X87_Float80 arctangent(const X87_Float80 &y, const X87_Float80 &x);
    // The C runtime's x87 asin and acos: for |x| < 1, an arctangent of x and
    // the square root of (1 + x)(1 - x) at the current precision with nearest
    // rounding; stored constants for x = +-1; the default NaN outside the
    // domain. Unless the control is 53-bit nearest, the runtime returns the
    // result rounded to binary64.
    X87_Float80 arcsine(const X87_Float80 &value);
    X87_Float80 arccosine(const X87_Float80 &value);
    X87_Float80 power2_minus1(const X87_Float80 &value);
    X87_Float80 log2_multiply(const X87_Float80 &x, const X87_Float80 &y);
    X87_Float80 round_integer(const X87_Float80 &value);
    X87_Float80 scale(const X87_Float80 &value, const X87_Float80 &exponent);
    X87_Float80 natural_logarithm_of_two() const;
    X87_Float80 logarithm2_of_e() const;
    X87_Float80 exponential(const X87_Float80 &value);
};

// Binary32 operations with a single nearest-even rounding.
float x87_add_float(float first, float second);
float x87_multiply_float(float first, float second);

#pragma pack(pop)

#endif
