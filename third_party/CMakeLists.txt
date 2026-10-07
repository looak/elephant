# Define a header-only INTERFACE libraries for third-party dependencies

# --- libpopcnt ----
# this was used in the intrinsics, maybe we re-enable it in the future
add_library(libpopcnt INTERFACE)

target_include_directories(libpopcnt
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/libpopcnt/inc
)

# Mark as SYSTEM to suppress warnings from third-party code
set_target_properties(libpopcnt PROPERTIES SYSTEM ON)

# --- spdlog ---- 
add_library(spdlog_local INTERFACE)
target_include_directories(spdlog_local
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include
)

# fmt (bundled in spdlog) requires /utf-8 on MSVC, upstream spdlog's CMake adds this for us
if(MSVC)
  target_compile_options(spdlog_local INTERFACE $<$<COMPILE_LANGUAGE:CXX>:/utf-8>)
endif()

# Mark as SYSTEM to suppress warnings from third-party code
set_target_properties(spdlog_local PROPERTIES SYSTEM ON)

# alias to make it look like the actual spdlog package -- for maintainability
add_library(spdlog ALIAS spdlog_local)