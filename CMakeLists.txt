cmake_minimum_required(VERSION 3.19)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

option(STATIC_CRT "Link with the static CRT (/MT)" OFF)

if (STATIC_CRT)
    cmake_policy(SET CMP0091 NEW)
    set(STATIC_CRT ON)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # clangd

project(Detect LANGUAGES CXX)

add_subdirectory(luau)

add_executable(Detect)

target_sources(Detect PRIVATE
    src/main.cpp
)

if (MSVC)
    list(APPEND OPTIONS /W3 /WX /D_CRT_SECURE_NO_WARNINGS)
    list(APPEND OPTIONS /MP) # Distribute compilation across multiple cores
else ()
    list(APPEND OPTIONS -Wall)
endif ()

target_compile_features(Detect PUBLIC cxx_std_20)
target_compile_options(Detect PRIVATE ${OPTIONS})
target_link_libraries(Detect PRIVATE Luau.Ast)
set_target_properties(Detect PROPERTIES OUTPUT_NAME luau-detect)
