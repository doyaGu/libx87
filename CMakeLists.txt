cmake_minimum_required(VERSION 3.12)
# Several consumers may carry their own submodule checkout in the same build.
if(TARGET libx87::x87)
    return()
endif()
project(libx87 VERSION 0.1.0 LANGUAGES CXX)

add_library(libx87 STATIC src/x87.cxx)
add_library(libx87::x87 ALIAS libx87)
target_compile_features(libx87 PRIVATE cxx_std_11)
target_include_directories(libx87 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
set_target_properties(libx87 PROPERTIES EXPORT_NAME x87 OUTPUT_NAME x87
    POSITION_INDEPENDENT_CODE ON CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)
add_subdirectory(third_party/softfloat)
if(MSVC)
    target_compile_options(libx87 PRIVATE /W3 /wd4146 /wd4244 /fp:precise)
else()
    target_compile_options(libx87 PRIVATE -ffp-contract=off)
endif()
include(CheckIPOSupported)
check_ipo_supported(RESULT LIBX87_IPO_SUPPORTED LANGUAGES CXX)
if(LIBX87_IPO_SUPPORTED)
    set_target_properties(libx87 PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/libx87-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/libx87-config.cmake"
    INSTALL_DESTINATION lib/cmake/libx87)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/libx87-config-version.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
export(TARGETS libx87 NAMESPACE libx87::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/libx87-targets.cmake")
option(LIBX87_INSTALL "Generate libx87 installation rules" ON)
if(LIBX87_INSTALL)
    include(GNUInstallDirs)
    configure_file(cmake/libx87.pc.in "${CMAKE_CURRENT_BINARY_DIR}/libx87.pc" @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libx87.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
    install(TARGETS libx87 EXPORT libx87-targets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(EXPORT libx87-targets NAMESPACE libx87:: DESTINATION lib/cmake/libx87)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libx87-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/libx87-config-version.cmake" DESTINATION lib/cmake/libx87)
    install(FILES third_party/softfloat/BOCHS-COPYING third_party/softfloat/BOCHS-LICENSE
        third_party/softfloat/cpu/softfloat3e/COPYING.txt third_party/softfloat/UPSTREAM.json
        LICENSE VXMATH-LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/libx87)
endif()
option(LIBX87_BUILD_TESTS "Build arithmetic and SoftFloat differential tests" OFF)
if(LIBX87_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
