cmake_minimum_required(VERSION 3.13)
project(bun C CXX)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OOZ_SOURCES
    bitknit.cpp
    bits_rev_table.h
    compr_entropy.cpp
    compr_entropy.h
    compr_kraken.cpp
    compr_kraken.h
    compr_leviathan.cpp
    compr_leviathan.h
    compr_match_finder.cpp
    compr_match_finder.h
    compr_mermaid.cpp
    compr_mermaid.h
    compr_multiarray.cpp
    compr_tans.cpp
    compr_util.h
    compress.cpp
    compress.h
    kraken.cpp
    log_lookup.h
    lzna.cpp
    match_hasher.h
    qsort.h
    targetver.h
)

set(LITE_SOURCES bitknit-lite.cpp kraken-lite.cpp lzna-lite.cpp)
add_library(libooz-lite SHARED ${LITE_SOURCES})
add_executable(ooz-validate ${LITE_SOURCES} validate.cpp)

target_include_directories(libooz-lite PUBLIC simde)
target_include_directories(ooz-validate PRIVATE simde)
target_link_libraries(ooz-validate PRIVATE PkgConfig::libsodium)

add_executable(ooz ${OOZ_SOURCES})

add_library(libooz SHARED ${OOZ_SOURCES})

target_compile_definitions(libooz PUBLIC OOZ_DYNAMIC)
target_compile_definitions(libooz PRIVATE OOZ_BUILD_DLL)

add_library(bunutil STATIC
    "fnv.cpp" "fnv.h"
    "util.cpp" "util.h"
    "utf.cpp" "utf.h"
    )
target_include_directories(bunutil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(libsodium REQUIRED IMPORTED_TARGET libsodium)
    target_link_libraries(bunutil
        PUBLIC
            PkgConfig::libsodium
            "unistring"
    )
endif()

add_library(libbun SHARED  "bun.cpp" "bun.h")
target_compile_definitions(libbun PUBLIC BUN_DYNAMIC)
target_compile_definitions(libbun PRIVATE BUN_BUILD_DLL)
target_include_directories(libbun INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libbun PUBLIC bunutil)
if (UNIX)
    target_link_libraries(libbun PRIVATE "-lstdc++fs" dl)
endif()

add_subdirectory(libpoe)

add_executable(bun_extract_file "bun_extract_file.cpp" "path_rep.cpp" "path_rep.h" "ggpk_vfs.cpp" "ggpk_vfs.h")
target_link_libraries(bun_extract_file PRIVATE libbun libpoe)
if (UNIX)
    target_link_libraries(bun_extract_file PRIVATE "-lstdc++fs")
endif()
