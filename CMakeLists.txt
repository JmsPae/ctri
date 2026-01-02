cmake_minimum_required(VERSION 4.0)
project(ctri C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)


add_subdirectory(external/minitrace)

add_executable(ctri src/main.c src/common.c src/index.c)
set_property(TARGET ctri PROPERTY C_STANDARD 23)

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

target_include_directories(ctri PRIVATE external)

find_package(Threads REQUIRED)
target_link_libraries(ctri PRIVATE Threads::Threads GL dl m X11 Xi Xcursor minitrace)
target_compile_definitions(ctri PRIVATE SOKOL_GLCORE)

