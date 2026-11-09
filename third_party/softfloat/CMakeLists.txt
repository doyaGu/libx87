include(TestBigEndian)
test_big_endian(LIBX87_SOFTFLOAT_BIG_ENDIAN)
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("int main() { return __builtin_clzll(1ULL); }"
    LIBX87_SOFTFLOAT_BUILTIN_CLZ)

configure_file(config.h.in generated/config.h)

# The vendored source set is immutable and recorded in UPSTREAM.json. Keep the
# generic integer implementation on every target; no native floating-point ABI
# or CPU emulator is part of this target.
file(GLOB arithmetic_sources CONFIGURE_DEPENDS
    cpu/softfloat3e/*.cc cpu/softfloat3e/8086-SSE/*.cc)
# These primitives are defined inline by primitives.h. Compiling their legacy
# out-of-line copies as well creates duplicate definitions in static archives.
foreach(primitive
    add128 sub128 eq128 le128 lt128 countLeadingZeros16 countLeadingZeros32
    mul128By32 mul64ByShifted32To128 shiftRightJam32 shiftRightJam64
    shiftRightJam64Extra shortShiftLeft128 shortShiftRight128
    shortShiftRightJam128 shortShiftRightJam128Extra shortShiftRightJam64
    shortShiftRightJam64Extra)
    list(REMOVE_ITEM arithmetic_sources
        "${CMAKE_CURRENT_SOURCE_DIR}/cpu/softfloat3e/s_${primitive}.cc")
endforeach()
if(LIBX87_SOFTFLOAT_BUILTIN_CLZ)
    list(REMOVE_ITEM arithmetic_sources
        "${CMAKE_CURRENT_SOURCE_DIR}/cpu/softfloat3e/s_countLeadingZeros64.cc")
endif()
set(numerical_sources ${arithmetic_sources}
    ${CMAKE_CURRENT_SOURCE_DIR}/cpu/fpu/f128_67.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/cpu/fpu/f2xm1.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/cpu/fpu/fpatan.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/cpu/fpu/fprem.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/cpu/fpu/fsincos.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/cpu/fpu/fyl2x.cc
)
target_sources(libx87 PRIVATE ${numerical_sources})
target_include_directories(libx87 PRIVATE
    include cpu cpu/softfloat3e/include cpu/fpu "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_definitions(libx87 PRIVATE
    SOFTFLOAT_FAST_INT64 SOFTFLOAT_FAST_DIV32TO16 SOFTFLOAT_FAST_DIV64TO32 INLINE_LEVEL=5)
# Direct SoftFloat access is only for numerical verification, not installed API.
add_library(libx87_softfloat INTERFACE)
add_library(libx87::softfloat ALIAS libx87_softfloat)
target_link_libraries(libx87_softfloat INTERFACE libx87::x87)
target_include_directories(libx87_softfloat INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/cpu"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpu/softfloat3e/include" "${CMAKE_CURRENT_SOURCE_DIR}/cpu/fpu"
    "${CMAKE_CURRENT_BINARY_DIR}/generated")
