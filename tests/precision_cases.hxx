#ifndef LIBX87_NUMERICS_X87_PRECISION_CASES_INCLUDED
#define LIBX87_NUMERICS_X87_PRECISION_CASES_INCLUDED

#include <libx87/x87.hxx>

static bool x87_float80_equal(const X87_Float80 &first, const X87_Float80 &second)
{
    return first.get_sign_exponent() == second.get_sign_exponent()
        && first.get_significand() == second.get_significand();
}

static int x87_check_x87_precision()
{
    X87_Arithmetic fp;
    const X87_Float80 one = fp.load(1);
    const X87_Float80 two = fp.load(2);
    const X87_Float80 zero = fp.load(0);
    int failures = 0;
    const X87_Float80 increment = X87_Float80::from_bits(0x3fc0, UINT64_C(0x8000000000000000));
    const X87_Float80 live_sum = fp.add(one, increment);
    failures += live_sum.get_significand() != UINT64_C(0x8000000000000001);
    failures += fp.store_double(live_sum) != 1.0;
    failures += !x87_float80_equal(fp.subtract(live_sum, one), increment);
    unsigned char bytes[10];
    live_sum.to_bytes(bytes);
    failures += bytes[0] != 1 || bytes[7] != 0x80 || bytes[8] != 0xff || bytes[9] != 0x3f;
    failures += !x87_float80_equal(X87_Float80::from_bytes(bytes), live_sum);

    X87_Arithmetic single(X87_Arithmetic::PRECISION_24, X87_Arithmetic::ROUND_UP);
    const X87_Float80 rounded = single.add(one, increment);
    failures += rounded.get_significand() != UINT64_C(0x8000010000000000);
    failures += !x87_float80_equal(fp.add(one, increment), live_sum);
    fp.divide(one, zero);
    failures += (fp.get_exception_flags() & X87_Arithmetic::EXCEPTION_DIVISION_BY_ZERO) == 0;
    failures += (single.get_exception_flags() & X87_Arithmetic::EXCEPTION_DIVISION_BY_ZERO) != 0;
    fp.clear_exception_flags();
    failures += fp.get_exception_flags() != 0;

    bool outside = true;
    failures += !x87_float80_equal(fp.sine(zero, &outside), zero) || outside;
    failures += !x87_float80_equal(fp.cosine(zero, &outside), one) || outside;
    failures += !x87_float80_equal(fp.square_root(fp.load(4)), two);
    failures += !x87_float80_equal(fp.power2_minus1(zero), zero);
    failures += !x87_float80_equal(fp.log2_multiply(two, two), two);
    failures += !x87_float80_equal(fp.scale(one, fp.load(3)), fp.load(8));
    failures += !x87_float80_equal(fp.round_integer(fp.load(1.5)), two);
    failures += !x87_float80_equal(fp.round_integer(fp.load(-1.5)), fp.load(-2));
    failures += fp.store_double(fp.arctangent(one, zero)) != 1.5707963267948966;
    failures += !x87_float80_equal(fp.arctangent(zero, one), zero);
    failures += fp.compare(one, two) != X87_Arithmetic::LESS;
    failures += fp.compare(two, one) != X87_Arithmetic::GREATER;
    failures += fp.compare(one, one) != X87_Arithmetic::EQUAL;

    // asin/acos as msvcrt.dll's _CIasin/_CIacos return them: rounded to
    // binary64 under 64-bit precision, the register value under 53-bit.
    struct UnitCase { double x; uint16_t asin_se[2]; uint64_t asin_sig[2]; uint16_t acos_se[2]; uint64_t acos_sig[2]; };
    static const UnitCase unit_cases[] = {
        {0.5, {0x3ffe, 0x3ffe}, {UINT64_C(0x860a91c16b9b3000), UINT64_C(0x860a91c16b9b2df2)},
            {0x3fff, 0x3fff}, {UINT64_C(0x860a91c16b9b3000), UINT64_C(0x860a91c16b9b2b3c)}},
        {-0.25, {0xbffd, 0xbffd}, {UINT64_C(0x815f4e705c3e6800), UINT64_C(0x815f4e705c3e67b9)},
            {0x3fff, 0x3fff}, {UINT64_C(0xe967ae3e38786000), UINT64_C(0xe967ae3e38785c23)}},
        {0.75, {0x3ffe, 0x3ffe}, {UINT64_C(0xd91a98ae3406e000), UINT64_C(0xd91a98ae3406de8e)},
            {0x3ffe, 0x3ffe}, {UINT64_C(0xb9051c960ecaa800), UINT64_C(0xb9051c960ecaa5db)}},
        {1.0, {0x3fff, 0x3fff}, {UINT64_C(0xc90fdaa22168c000), UINT64_C(0xc90fdaa22168c235)},
            {0, 0}, {0, 0}},
        {-1.0, {0xbfff, 0xbfff}, {UINT64_C(0xc90fdaa22168c000), UINT64_C(0xc90fdaa22168c235)},
            {0x4000, 0x4000}, {UINT64_C(0xc90fdaa22168c000), UINT64_C(0xc90fdaa22168c235)}},
    };
    X87_Arithmetic extended(X87_Arithmetic::PRECISION_64, X87_Arithmetic::ROUND_NEAREST);
    X87_Arithmetic standard(X87_Arithmetic::PRECISION_53, X87_Arithmetic::ROUND_NEAREST);
    for(unsigned i = 0; i < sizeof(unit_cases) / sizeof(unit_cases[0]); ++i)
    {
        const UnitCase &c = unit_cases[i];
        X87_Arithmetic *selection[2] = {&extended, &standard};
        for(int p = 0; p < 2; ++p)
        {
            const X87_Float80 x = selection[p]->load(c.x);
            failures += !x87_float80_equal(selection[p]->arcsine(x), X87_Float80::from_bits(c.asin_se[p], c.asin_sig[p]));
            failures += !x87_float80_equal(selection[p]->arccosine(x), X87_Float80::from_bits(c.acos_se[p], c.acos_sig[p]));
        }
    }
    failures += !x87_float80_equal(extended.arcsine(two), X87_Float80::from_bits(0xffff, UINT64_C(0xc000000000000000)));

    return failures;
}

#endif
