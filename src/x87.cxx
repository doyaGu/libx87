// Extended-precision arithmetic, rounding and exception status.

#include <libx87/x87.hxx>

#include <x87_softfloat.hxx>
#include <assert.h>
#include <new>
#include <string.h>

typedef char X87_Status_Size_Check[sizeof(softfloat_status_t) <= 16 ? 1 : -1];

static extFloat80_t x87_input(const X87_Float80 &value)
{
    extFloat80_t result;
    result.signif = value.get_significand();
    result.signExp = value.get_sign_exponent();
    return result;
}

static X87_Float80 x87_output(extFloat80_t value)
{
    return X87_Float80::from_bits(value.signExp, value.signif);
}

X87_Float80 X87_Float80::from_bytes(const unsigned char *bytes)
{
    uint64_t fraction = 0;
    for(unsigned i = 0; i < 8; ++i)
        fraction |= static_cast<uint64_t>(bytes[i]) << (i * 8);
    const uint16_t exponent = static_cast<uint16_t>(bytes[8] | (bytes[9] << 8));
    return from_bits(exponent, fraction);
}

void X87_Float80::to_bytes(unsigned char *bytes) const
{
    for(unsigned i = 0; i < 8; ++i)
        bytes[i] = static_cast<unsigned char>(significand >> (i * 8));
    bytes[8] = static_cast<unsigned char>(sign_exponent);
    bytes[9] = static_cast<unsigned char>(sign_exponent >> 8);
}

X87_Arithmetic::X87_Arithmetic(Precision precision, Rounding rounding,
    bool arithmetic, bool transcendental)
    : fast_path(false), fast_inexact(false), native_arithmetic(arithmetic),
      native_transcendental(transcendental)
{
    softfloat_status_t *status = new(get_status()) softfloat_status_t();
    status->softfloat_exceptionMasks = softfloat_all_exceptions_mask;
    set_precision(precision);
    set_rounding(rounding);
}


void X87_Arithmetic::set_precision(Precision precision)
{
    assert(precision == PRECISION_24 || precision == PRECISION_53 || precision == PRECISION_64);
    static_cast<softfloat_status_t *>(get_status())->extF80_roundingPrecision =
        precision == PRECISION_24 ? 32 : precision == PRECISION_53 ? 64 : 80;
    update_fast_path();
}

void X87_Arithmetic::set_rounding(Rounding rounding)
{
    assert(rounding >= ROUND_NEAREST && rounding <= ROUND_TO_ZERO);
    static_cast<softfloat_status_t *>(get_status())->softfloat_roundingMode =
        static_cast<uint8_t>(rounding);
    update_fast_path();
}

void X87_Arithmetic::update_fast_path()
{
    const softfloat_status_t *status = static_cast<const softfloat_status_t *>(get_status());
    fast_path = status->extF80_roundingPrecision == 80
        && status->softfloat_roundingMode == softfloat_round_near_even;
}

unsigned X87_Arithmetic::get_exception_flags() const
{
    return (static_cast<unsigned>(static_cast<const softfloat_status_t *>(get_status())->softfloat_exceptionFlags) & 0x3f)
        | (fast_inexact ? static_cast<unsigned>(EXCEPTION_INEXACT_RESULT) : 0u);
}

void X87_Arithmetic::clear_exception_flags()
{
    static_cast<softfloat_status_t *>(get_status())->softfloat_exceptionFlags = 0;
    fast_inexact = false;
}

X87_Float80 X87_Arithmetic::load_general(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return x87_output(f32_to_extF80(bits, static_cast<softfloat_status_t *>(get_status())));
}

X87_Float80 X87_Arithmetic::load_general(double value)
{
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return x87_output(f64_to_extF80(bits, static_cast<softfloat_status_t *>(get_status())));
}

X87_Float80 X87_Arithmetic::load(int32_t value)
{
    return x87_output(i32_to_extF80(value));
}

float X87_Arithmetic::store_float_general(const X87_Float80 &value)
{
    const uint32_t bits = extF80_to_f32(x87_input(value), static_cast<softfloat_status_t *>(get_status()));
    float result;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

double X87_Arithmetic::store_double_general(const X87_Float80 &value)
{
    const uint64_t bits = extF80_to_f64(x87_input(value), static_cast<softfloat_status_t *>(get_status()));
    double result;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

#if defined(_MSC_VER) && defined(_M_IX86)
// Only transport uses the native ABI; arithmetic remains in the integer
// numerical backend. The byte encoding avoids depending on object padding.
static __declspec(naked) double __cdecl x87_return_double(const unsigned char *)
{
    __asm {
        mov eax, [esp+4]
        fld tbyte ptr [eax]
        ret
    }
}

static __declspec(naked) float __cdecl x87_return_float(const unsigned char *)
{
    __asm {
        mov eax, [esp+4]
        fld tbyte ptr [eax]
        ret
    }
}
#endif

double X87_Arithmetic::return_double(const X87_Float80 &value)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    unsigned char bytes[10];
    value.to_bytes(bytes);
    return x87_return_double(bytes);
#else
    return store_double(value);
#endif
}

float X87_Arithmetic::return_float(const X87_Float80 &value)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    unsigned char bytes[10];
    value.to_bytes(bytes);
    return x87_return_float(bytes);
#else
    return store_float(value);
#endif
}

X87_Float80 X87_Arithmetic::add_general(const X87_Float80 &first, const X87_Float80 &second)
{
    return x87_output(extF80_add(x87_input(first), x87_input(second),
        static_cast<softfloat_status_t *>(get_status())));
}

X87_Float80 X87_Arithmetic::subtract_general(const X87_Float80 &first, const X87_Float80 &second)
{
    return x87_output(extF80_sub(x87_input(first), x87_input(second),
        static_cast<softfloat_status_t *>(get_status())));
}

X87_Float80 X87_Arithmetic::multiply_general(const X87_Float80 &first, const X87_Float80 &second)
{
    return x87_output(extF80_mul(x87_input(first), x87_input(second),
        static_cast<softfloat_status_t *>(get_status())));
}

X87_Float80 X87_Arithmetic::divide_general(const X87_Float80 &first, const X87_Float80 &second)
{
    return x87_output(extF80_div(x87_input(first), x87_input(second),
        static_cast<softfloat_status_t *>(get_status())));
}

X87_Float80 X87_Arithmetic::square_root(const X87_Float80 &value)
{
#if defined(_M_IX86) || defined(__i386__)
    if(native_arithmetic && use_native_path()) return x87_native_square_root(value);
#endif
    return x87_output(extF80_sqrt(x87_input(value), static_cast<softfloat_status_t *>(get_status())));
}

X87_Float80 X87_Arithmetic::absolute(const X87_Float80 &value)
{
    return X87_Float80::from_bits(value.get_sign_exponent() & 0x7fff, value.get_significand());
}

X87_Float80 X87_Arithmetic::negate(const X87_Float80 &value)
{
    return X87_Float80::from_bits(value.get_sign_exponent() ^ 0x8000, value.get_significand());
}

X87_Arithmetic::Relation X87_Arithmetic::compare(
    const X87_Float80 &first, const X87_Float80 &second, bool quiet)
{
    return static_cast<Relation>(extF80_compare(x87_input(first), x87_input(second), quiet,
        static_cast<softfloat_status_t *>(get_status())));
}

X87_Float80 X87_Arithmetic::sine(const X87_Float80 &value, bool *out_of_range)
{
#if defined(_M_IX86) || defined(__i386__)
    if(native_transcendental && use_native_path())
    {
        if(out_of_range) *out_of_range = (value.get_sign_exponent() & 0x7fff) >= 0x3fff + 63;
        return x87_native_sine(value);
    }
#endif
    extFloat80_t result = x87_input(value);
    const int range = fsin(result, *static_cast<softfloat_status_t *>(get_status()));
    if(out_of_range) *out_of_range = range != 0;
    return x87_output(result);
}

X87_Float80 X87_Arithmetic::cosine(const X87_Float80 &value, bool *out_of_range)
{
#if defined(_M_IX86) || defined(__i386__)
    if(native_transcendental && use_native_path())
    {
        if(out_of_range) *out_of_range = (value.get_sign_exponent() & 0x7fff) >= 0x3fff + 63;
        return x87_native_cosine(value);
    }
#endif
    extFloat80_t result = x87_input(value);
    const int range = fcos(result, *static_cast<softfloat_status_t *>(get_status()));
    if(out_of_range) *out_of_range = range != 0;
    return x87_output(result);
}

X87_Float80 X87_Arithmetic::arctangent(const X87_Float80 &y, const X87_Float80 &x)
{
#if defined(_M_IX86) || defined(__i386__)
    if(native_transcendental && use_native_path()) return x87_native_arctangent(y, x);
#endif
    return x87_output(fpatan(x87_input(x), x87_input(y),
        *static_cast<softfloat_status_t *>(get_status())));
}

// The runtime classifies the binary64 copy of its argument and evaluates the
// extended argument itself. +-1 and the default NaN are its stored constants.
enum X87_Domain { X87_INSIDE, X87_PLUS_ONE, X87_MINUS_ONE, X87_NAN, X87_OUTSIDE };

static X87_Domain x87_unit_domain(X87_Arithmetic &fp, const X87_Float80 &value)
{
    const double rounded = fp.store_double(value);
    uint64_t bits;
    memcpy(&bits, &rounded, sizeof(bits));
    const unsigned exponent = static_cast<unsigned>(bits >> 52) & 0x7ff;
    const uint64_t fraction = bits & UINT64_C(0xfffffffffffff);
    if(exponent < 0x3ff) return X87_INSIDE;
    if(exponent == 0x3ff && fraction == 0) return (bits >> 63) ? X87_MINUS_ONE : X87_PLUS_ONE;
    if(exponent == 0x7ff && fraction != 0) return X87_NAN;
    return X87_OUTSIDE;
}

static X87_Float80 x87_unit_root(X87_Arithmetic &fp, const X87_Float80 &value)
{
    const X87_Float80 one = fp.load(1.0);
    return fp.square_root(fp.multiply(fp.add(one, value), fp.subtract(one, value)));
}

static X87_Float80 x87_quiet(const X87_Float80 &value)
{
    return X87_Float80::from_bits(value.get_sign_exponent(), value.get_significand() | (UINT64_C(1) << 62));
}

static const uint64_t x87_pi_significand = UINT64_C(0xc90fdaa22168c235);

// The runtime evaluates with the caller's precision and nearest rounding. Unless
// the caller's control word is the default (53-bit, nearest), its exit hands an
// inexact result to the exception path, which stores and reloads it as
// binary64; the precision flag is sticky, so a running program always takes it.
static X87_Arithmetic::Precision x87_precision(const softfloat_status_t *status)
{
    return status->extF80_roundingPrecision == 32 ? X87_Arithmetic::PRECISION_24
        : status->extF80_roundingPrecision == 64 ? X87_Arithmetic::PRECISION_53 : X87_Arithmetic::PRECISION_64;
}

static X87_Float80 x87_runtime_result(softfloat_status_t *caller, X87_Arithmetic &runtime,
    const X87_Float80 &result)
{
    X87_Float80 rounded = result;
    if(caller->extF80_roundingPrecision != 64 || caller->softfloat_roundingMode != softfloat_round_near_even)
        rounded = runtime.load(runtime.store_double(result));
    softfloat_raiseFlags(caller, runtime.get_exception_flags());
    return rounded;
}

X87_Float80 X87_Arithmetic::arcsine(const X87_Float80 &value)
{
    softfloat_status_t *status = static_cast<softfloat_status_t *>(get_status());
    X87_Arithmetic runtime(x87_precision(status), ROUND_NEAREST,
        native_arithmetic, native_transcendental);
    X87_Float80 result;
    switch(x87_unit_domain(runtime, value))
    {
    case X87_INSIDE: result = runtime.arctangent(value, x87_unit_root(runtime, value)); break;
    case X87_PLUS_ONE: result = X87_Float80::from_bits(0x3fff, x87_pi_significand); break;
    case X87_MINUS_ONE: result = X87_Float80::from_bits(0xbfff, x87_pi_significand); break;
    case X87_NAN:
        softfloat_raiseFlags(status, runtime.get_exception_flags());
        return x87_quiet(value);
    default:
        softfloat_raiseFlags(status, softfloat_flag_invalid | runtime.get_exception_flags());
        return X87_Float80::from_bits(0xffff, UINT64_C(0xc000000000000000));
    }
    return x87_runtime_result(status, runtime, result);
}

X87_Float80 X87_Arithmetic::arccosine(const X87_Float80 &value)
{
    softfloat_status_t *status = static_cast<softfloat_status_t *>(get_status());
    X87_Arithmetic runtime(x87_precision(status), ROUND_NEAREST,
        native_arithmetic, native_transcendental);
    X87_Float80 result;
    switch(x87_unit_domain(runtime, value))
    {
    case X87_INSIDE: result = runtime.arctangent(x87_unit_root(runtime, value), value); break;
    case X87_PLUS_ONE: result = X87_Float80::from_bits(0, 0); break;
    case X87_MINUS_ONE: result = X87_Float80::from_bits(0x4000, x87_pi_significand); break;
    case X87_NAN:
        softfloat_raiseFlags(status, runtime.get_exception_flags());
        return x87_quiet(value);
    default:
        softfloat_raiseFlags(status, softfloat_flag_invalid | runtime.get_exception_flags());
        return X87_Float80::from_bits(0xffff, UINT64_C(0xc000000000000000));
    }
    return x87_runtime_result(status, runtime, result);
}

X87_Float80 X87_Arithmetic::power2_minus1(const X87_Float80 &value)
{
#if defined(_M_IX86) || defined(__i386__)
    if(native_transcendental && use_native_path()) return x87_native_power2_minus1(value);
#endif
    return x87_output(f2xm1(x87_input(value), *static_cast<softfloat_status_t *>(get_status())));
}

X87_Float80 X87_Arithmetic::log2_multiply(const X87_Float80 &x, const X87_Float80 &y)
{
#if defined(_M_IX86) || defined(__i386__)
    if(native_transcendental && use_native_path()) return x87_native_log2_multiply(x, y);
#endif
    return x87_output(fyl2x(x87_input(x), x87_input(y),
        *static_cast<softfloat_status_t *>(get_status())));
}

X87_Float80 X87_Arithmetic::round_integer(const X87_Float80 &value)
{
#if defined(_M_IX86) || defined(__i386__)
    if(native_arithmetic && use_native_path()) return x87_native_round_integer(value);
#endif
    return x87_output(extF80_roundToInt(x87_input(value), static_cast<softfloat_status_t *>(get_status())));
}

X87_Float80 X87_Arithmetic::scale(const X87_Float80 &value, const X87_Float80 &exponent)
{
#if defined(_M_IX86) || defined(__i386__)
    if(native_arithmetic && use_native_path()) return x87_native_scale(value, exponent);
#endif
    return x87_output(extF80_scale(x87_input(value), x87_input(exponent),
        static_cast<softfloat_status_t *>(get_status())));
}

// Constant loads round the internal constants according to RC, independently
// of arithmetic precision, and do not raise the inexact-result flag.
X87_Float80 X87_Arithmetic::natural_logarithm_of_two() const
{
    const unsigned mode = static_cast<const softfloat_status_t *>(get_status())->softfloat_roundingMode;
    const uint64_t significand = UINT64_C(0xb17217f7d1cf79ac) -
        (mode == ROUND_DOWN || mode == ROUND_TO_ZERO);
    return X87_Float80::from_bits(0x3ffe, significand);
}

X87_Float80 X87_Arithmetic::logarithm2_of_e() const
{
    const unsigned mode = static_cast<const softfloat_status_t *>(get_status())->softfloat_roundingMode;
    const uint64_t significand = UINT64_C(0xb8aa3b295c17f0bc) -
        (mode == ROUND_DOWN || mode == ROUND_TO_ZERO);
    return X87_Float80::from_bits(0x3fff, significand);
}

X87_Float80 X87_Arithmetic::exponential(const X87_Float80 &value)
{
    const X87_Float80 exponent = multiply(value, logarithm2_of_e());
    const X87_Float80 integral = round_integer(exponent);
    return scale(add(power2_minus1(subtract(exponent, integral)), load(1.0)), integral);
}

void X87_Arithmetic::raise_exception_flags(unsigned flags)
{
    softfloat_raiseFlags(static_cast<softfloat_status_t *>(get_status()), flags & 0x3f);
}

static float x87_binary32(float first, float second, bool multiply)
{
    softfloat_status_t status = {};
    status.softfloat_exceptionMasks = softfloat_all_exceptions_mask;
    status.softfloat_roundingMode = softfloat_round_near_even;
    uint32_t a, b;
    memcpy(&a, &first, sizeof(a));
    memcpy(&b, &second, sizeof(b));
    const uint32_t bits = multiply ? f32_mul(a, b, &status) : f32_add(a, b, &status);
    float result;
    memcpy(&result, &bits, sizeof(result));
    return result;
}
float x87_add_float(float first, float second) { return x87_binary32(first, second, false); }
float x87_multiply_float(float first, float second) { return x87_binary32(first, second, true); }
