#include <libx87/x87.hxx>

static unsigned short control_word()
{
    unsigned short word;
#if defined(_MSC_VER)
    __asm fnstcw word
#else
    __asm__ __volatile__("fnstcw %0" : "=m"(word));
#endif
    return word;
}
static void set_control(unsigned short word)
{
#if defined(_MSC_VER)
    __asm fldcw word
#else
    __asm__ __volatile__("fldcw %0" : : "m"(word));
#endif
}
static bool same(const X87_Float80 &a, const X87_Float80 &b)
{
    return a.get_significand() == b.get_significand()
        && a.get_sign_exponent() == b.get_sign_exponent();
}
int main()
{
    const unsigned short saved = control_word();
    set_control(static_cast<unsigned short>((saved & ~0x0f00) | 0x033f));
    X87_Arithmetic software;
    X87_Arithmetic arithmetic(X87_Arithmetic::PRECISION_64, X87_Arithmetic::ROUND_NEAREST, true, false);
    X87_Arithmetic transcendental(X87_Arithmetic::PRECISION_64, X87_Arithmetic::ROUND_NEAREST, false, true);
    const X87_Float80 one = software.load(1), three = software.load(3);
    int failures = !same(arithmetic.divide(one, three), software.divide(one, three));
    failures += arithmetic.get_exception_flags() != 0;
    failures += software.get_exception_flags() != X87_Arithmetic::EXCEPTION_INEXACT_RESULT;
    failures += !same(transcendental.divide(one, three), software.divide(one, three));
    failures += transcendental.get_exception_flags() != X87_Arithmetic::EXCEPTION_INEXACT_RESULT;
    const X87_Float80 zero = software.load(0);
    failures += !same(transcendental.sine(zero), zero);
    failures += !same(transcendental.cosine(zero), one);
    failures += !same(transcendental.arctangent(zero, one), zero);
    failures += !same(arithmetic.square_root(software.load(4)), software.load(2));
    // Native control is checked again after constructing the contexts.
    set_control(static_cast<unsigned short>((saved & ~0x0f00) | 0x023f));
    arithmetic.clear_exception_flags();
    software.clear_exception_flags();
    failures += !same(arithmetic.divide(one, three), software.divide(one, three));
    failures += arithmetic.get_exception_flags() != software.get_exception_flags();
    // An explicit precision/rounding selection also takes the software path.
    arithmetic.set_precision(X87_Arithmetic::PRECISION_24);
    arithmetic.set_rounding(X87_Arithmetic::ROUND_UP);
    software.set_precision(X87_Arithmetic::PRECISION_24);
    software.set_rounding(X87_Arithmetic::ROUND_UP);
    failures += !same(arithmetic.divide(one, three), software.divide(one, three));
    set_control(saved);
    return failures ? 1 : 0;
}
