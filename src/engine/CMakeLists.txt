add_library(${ENGINE_LIB} STATIC)

include(sourcelist.cmake)

target_sources(${ENGINE_LIB}
    PRIVATE
        ${ENGINE_SOURCE}
    PUBLIC
        ${ENGINE_SOURCE_INCLUDE}
)

target_include_directories(${ENGINE_LIB}
    PUBLIC
        ${ENGINE_INC_DIR}
)