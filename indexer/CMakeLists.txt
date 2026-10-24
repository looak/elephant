# Elephant Gambit Chess Engine - a Chess AI
# Copyright (C) 2021-2026  Alexander Loodin Ek

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# PhantIndexer builds phant-indexer, which replays PGN games and writes an index entry per game for nyayo's game
# catalogue. PhantIndexerLib is the replay & the entry, linked by the executable and by ElephantTest.
set(INDEXER_LIB PhantIndexerLib)
set(INDEXER_EXE PhantIndexer)

# the engine's parsers report an illegal move by throwing, see game_index.cpp.
if(NOT EXCEPTIONS_ENABLED)
    message(STATUS "## PhantIndexer: OFF, it needs EXCEPTIONS_ENABLED")
    return()
endif()
message(STATUS "## PhantIndexer: ON")

# bz2, gzip & zstd input: bzip2's, zlib's and zstd's own sources at a pinned release, built here as static libraries
# with only what decompressing needs, linked into PhantIndexerLib alone, so the release binary needs nothing installed
# and the engine never sees them. SOURCE_SUBDIR names no directory, which keeps their own CMake builds out.
enable_language(C)
include(FetchContent)
FetchContent_Declare(zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG        v1.3.1
    SOURCE_SUBDIR  not-built-by-its-cmake)
FetchContent_Declare(bzip2
    GIT_REPOSITORY https://gitlab.com/bzip2/bzip2.git
    GIT_TAG        bzip2-1.0.8
    SOURCE_SUBDIR  not-built-by-its-cmake)
FetchContent_Declare(zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG        v1.5.7
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  not-built-by-its-cmake)
FetchContent_MakeAvailable(zlib bzip2 zstd)

add_library(indexer_zlib STATIC
    ${zlib_SOURCE_DIR}/adler32.c ${zlib_SOURCE_DIR}/crc32.c ${zlib_SOURCE_DIR}/inffast.c
    ${zlib_SOURCE_DIR}/inflate.c ${zlib_SOURCE_DIR}/inftrees.c ${zlib_SOURCE_DIR}/zutil.c)
target_include_directories(indexer_zlib SYSTEM PUBLIC ${zlib_SOURCE_DIR})
add_library(indexer_bzip2 STATIC
    ${bzip2_SOURCE_DIR}/blocksort.c ${bzip2_SOURCE_DIR}/bzlib.c ${bzip2_SOURCE_DIR}/compress.c
    ${bzip2_SOURCE_DIR}/crctable.c ${bzip2_SOURCE_DIR}/decompress.c ${bzip2_SOURCE_DIR}/huffman.c
    ${bzip2_SOURCE_DIR}/randtable.c)
target_include_directories(indexer_bzip2 SYSTEM PUBLIC ${bzip2_SOURCE_DIR})
# zstd's decompression: lib/common & lib/decompress, without the x86-64 assembly (a .S file MSVC can't build) and
# without the pre-1.0 formats.
set(ZSTD_LIB ${zstd_SOURCE_DIR}/lib)
add_library(indexer_zstd STATIC
    ${ZSTD_LIB}/common/debug.c ${ZSTD_LIB}/common/entropy_common.c ${ZSTD_LIB}/common/error_private.c
    ${ZSTD_LIB}/common/fse_decompress.c ${ZSTD_LIB}/common/pool.c ${ZSTD_LIB}/common/threading.c
    ${ZSTD_LIB}/common/xxhash.c ${ZSTD_LIB}/common/zstd_common.c
    ${ZSTD_LIB}/decompress/huf_decompress.c ${ZSTD_LIB}/decompress/zstd_ddict.c
    ${ZSTD_LIB}/decompress/zstd_decompress.c ${ZSTD_LIB}/decompress/zstd_decompress_block.c)
target_include_directories(indexer_zstd SYSTEM PUBLIC ${ZSTD_LIB})
target_compile_definitions(indexer_zstd PRIVATE ZSTD_DISABLE_ASM ZSTD_LEGACY_SUPPORT=0)
foreach(compression indexer_zlib indexer_bzip2 indexer_zstd)
    target_compile_options(${compression} PRIVATE $<IF:$<BOOL:${MSVC}>,/w,-w>)
    set_target_properties(${compression} PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
endforeach()

add_library(${INDEXER_LIB} STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/game_index.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/input.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_index.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input.cpp)
target_compile_features(${INDEXER_LIB} PUBLIC cxx_std_20)
set_target_properties(${INDEXER_LIB} PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
target_include_directories(${INDEXER_LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(${INDEXER_LIB} PUBLIC engine spdlog
    PRIVATE common_warnings indexer_zlib indexer_bzip2 indexer_zstd)
# bzlib.h includes windows.h, whose min & max macros break std::min.
target_compile_definitions(${INDEXER_LIB} PRIVATE $<$<BOOL:${WIN32}>:NOMINMAX WIN32_LEAN_AND_MEAN>)

add_executable(${INDEXER_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
set_target_properties(${INDEXER_EXE} PROPERTIES
    OUTPUT_NAME phant-indexer
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
target_link_libraries(${INDEXER_EXE} PRIVATE ${INDEXER_LIB} common_warnings)
