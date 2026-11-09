foreach(name precision softfloat fast_path)
    add_executable(libx87_${name}_test ${name}_test.cxx)
    target_link_libraries(libx87_${name}_test PRIVATE libx87::x87)
    if(NOT name STREQUAL "precision")
        target_link_libraries(libx87_${name}_test PRIVATE libx87::softfloat)
        target_compile_features(libx87_${name}_test PRIVATE cxx_std_11)
    else()
        set_target_properties(libx87_${name}_test PROPERTIES
            CXX_STANDARD 98 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    endif()
    add_test(NAME libx87_${name} COMMAND libx87_${name}_test)
endforeach()

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#if !defined(_M_IX86) && !defined(__i386__)
#error not x86-32
#endif
int main() { return 0; }" LIBX87_TARGET_IS_X86_32)
if(LIBX87_TARGET_IS_X86_32)
    add_executable(libx87_native_test native_test.cxx)
    target_link_libraries(libx87_native_test PRIVATE libx87::x87)
    add_test(NAME libx87_native COMMAND libx87_native_test)
endif()
