# Define a header-only INTERFACE libraries for third-party dependencies

# --- libpopcnt ----
# this was used in the intrinsics, maybe we re-enable it in the future
add_library(libpopcnt INTERFACE)

target_include_directories(libpopcnt
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/libpopcnt/inc
)

# --- spdlog ---- 
add_library(spdlog_local INTERFACE)
target_include_directories(spdlog_local
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include
)

# alias to make it look like the actual spdlog package -- for maintainability
add_library(spdlog ALIAS spdlog_local)