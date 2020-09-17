cmake_minimum_required(VERSION 3.15)
project(bun C CXX)
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

add_library(libbun SHARED  "bun.cpp" "bun.h")
target_compile_definitions(libbun PUBLIC BUN_DYNAMIC)
target_compile_definitions(libbun PRIVATE BUN_BUILD_DLL)
target_include_directories(libbun INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libbun PUBLIC bunutil)

add_executable(bun_extract_file "bun_extract_file.cpp")
target_link_libraries(bun_extract_file PRIVATE libbun)
