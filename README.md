# libx87

Portable extended-precision arithmetic shared by VxMath and IVP. Values use an
explicit 64-bit significand and 16-bit sign/exponent, independent of host
`long double`. Serialize with `to_bytes`/`from_bytes` for the ten-byte
little-endian x87 encoding; object padding is not serialized.

```cpp
#include <libx87/x87.hxx>
X87_Arithmetic fp;
X87_Float80 quotient = fp.divide(fp.load(1), fp.load(3));
double result = fp.store_double(quotient);
```

`X87_Arithmetic` owns precision (24, 53 or 64 significand bits), rounding
(nearest-even, down, up or toward zero) and accumulated masked exception flags.
Loads are exact; stores round at explicit binary32/binary64 write boundaries.
Operations allocate no memory. The public header supports C++98 and preserves
its layout under caller packing directives; implementation sources use C++11.
The library does not own thread control words or host callbacks.

Normal 64-bit nearest-even arithmetic uses inline integer paths. Other inputs
and modes use the pinned Bochs SoftFloat and x87 transcendental sources.
Arithmetic includes square root, comparisons, sine/cosine, arctangent,
logarithms, exponential and the original runtime asin/acos sequences.
The two optional constructor booleans select native arithmetic and native
transcendentals independently on 32-bit x86, checking native control on each
operation. Other processors always use software. Native execution raises host
exception flags; software execution accumulates the object's flags.

```sh
cmake -S . -B build -DLIBX87_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix /your/prefix
```

Use `add_subdirectory` or `find_package(libx87 CONFIG REQUIRED)`, then link
`libx87::x87`. Multiple consumers may each carry this repository as a submodule;
only the first checkout added to a build creates the target. Keep their pinned
commits synchronized. `LIBX87_INSTALL` defaults to `ON`; set it to `OFF` only
when no installed consumer needs the package. The build directory also
contains a package config usable through `libx87_DIR`.

Tests cover precision, rounding, encoding, transcendental results and exception
flags, including randomized comparisons of fast paths with SoftFloat.
`libx87::softfloat` exposes private upstream headers for build-tree verification;
it is not installed. `x87_add_float` and `x87_multiply_float` provide single
nearest-even binary32 operations for scalar SIMD equivalents.

Upstream source hashes and licenses are retained in `third_party/softfloat`.
See [LICENSE](LICENSE) and [the source manifest](third_party/softfloat/UPSTREAM.json).
