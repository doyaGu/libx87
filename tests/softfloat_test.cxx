#include "softfloat_cases.hxx"
#include <cstdio>

int main()
{
    const int failures = x87_check_softfloat_arithmetic();
    std::printf("software arithmetic/transcendental/precision failures: %d\n", failures);
    return failures ? 1 : 0;
}
