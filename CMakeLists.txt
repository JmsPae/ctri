cmake_minimum_required(VERSION 3.30)
project(ctri C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)

add_executable(
    ctri 
    src/main.c src/world.c src/common.c src/index.c
)

# No particular reason, it's been 3 years and I wanted a proper bool.
set_property(TARGET ctri PROPERTY C_STANDARD 23)

target_include_directories(ctri PRIVATE external)

add_subdirectory(external/minitrace)

# I trust you, b2d
set(BOX2D_SAMPLES OFF)
set(BOX2D_UNIT_TESTS OFF)
add_subdirectory(external/box2d)

if (EMSCRIPTEN)
    target_compile_definitions(ctri PRIVATE SOKOL_GLES3)

    target_compile_options(ctri PRIVATE
        -msse -msse2 -msimd128
        $<$<CONFIG:Debug>:
            -O0
        >
        $<$<CONFIG:Release>:
            -Os
        >
    )

    target_link_options(ctri PRIVATE -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sUSE_WEBGL2=1 -sALLOW_MEMORY_GROWTH -sMALLOC=emmalloc --closure=1)

    # Keeping minitrace just to keep the macros from error'ing
    target_link_libraries(ctri PRIVATE box2d minitrace)

else()
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        if(supported)
            message(STATUS "IPO / LTO enabled")
            set_property(TARGET ctri PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
        else()
            message(STATUS "IPO / LTO not supported: <${error}>")
        endif()
    else()
        target_compile_definitions(ctri PRIVATE MTR_ENABLED)
    endif()
    target_compile_definitions(ctri PRIVATE SOKOL_GLCORE)
    target_compile_options(ctri PRIVATE
        $<$<C_COMPILER_ID:GNU,Clang>:-msse2>
        $<$<C_COMPILER_ID:GNU,Clang>:-msse>
        $<$<C_COMPILER_ID:MSVC>:/arch:SSE2>
        $<$<C_COMPILER_ID:MSVC>:/arch:SSE>
        $<$<CONFIG:Debug>:
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-O0>
            $<$<CXX_COMPILER_ID:MSVC>:/Od>
        >
        $<$<CONFIG:Release>:
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-O3>
            $<$<CXX_COMPILER_ID:MSVC>:/O3>
        >
    )

    find_package(Threads REQUIRED)
    target_link_libraries(ctri PRIVATE Threads::Threads GL dl m X11 Xi Xcursor box2d minitrace)

endif()


