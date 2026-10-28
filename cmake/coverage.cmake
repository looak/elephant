# Coverage instrumentation (ELEPHANT_COVERAGE), linked as the coverage_flags interface library by engine, cli,
# PhantIndexerLib and ElephantTest, never by googletest or third_party. Empty when the option is off. test/coverage.ps1
# and test/coverage.sh build with it and write the report.

add_library(coverage_flags INTERFACE)

if(ELEPHANT_COVERAGE)
    # atomic counters, the search's threads race on them otherwise, and gcov chokes on the negative counts that come out.
    target_compile_options(coverage_flags INTERFACE -fprofile-update=atomic)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # gcc: .gcno next to the objects, .gcda written when the test exits, read by gcovr.
        target_compile_options(coverage_flags INTERFACE --coverage)
        target_link_options(coverage_flags INTERFACE --coverage)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # clang & clang-cl: source based coverage, a .profraw per run (LLVM_PROFILE_FILE), read by llvm-cov.
        target_compile_options(coverage_flags INTERFACE -fprofile-instr-generate -fcoverage-mapping)
        if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
            # clang-cl's objects ask for clang_rt.profile-x86_64.lib by name, CMake links with lld-link or link.exe
            # directly, which only find it with the runtime directory on the library path.
            execute_process(COMMAND ${CMAKE_CXX_COMPILER} /clang:-print-runtime-dir
                OUTPUT_VARIABLE CLANG_RUNTIME_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
            target_link_directories(coverage_flags INTERFACE ${CLANG_RUNTIME_DIR})
        else()
            target_link_options(coverage_flags INTERFACE -fprofile-instr-generate)
        endif()
    else()
        message(FATAL_ERROR "ELEPHANT_COVERAGE needs gcc, clang or clang-cl, not ${CMAKE_CXX_COMPILER_ID}")
    endif()
    message(STATUS "## Coverage: ON")
endif()
