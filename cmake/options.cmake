set(DEBUG_TRANSITION_TABLE OFF CACHE STRING "Enable debug output for transition table" FORCE)
set(DEBUG_LOGGING_ENABLED OFF CACHE STRING "Enable debug logging" FORCE)
set(LOGGING_ENABLED ON CACHE STRING "Enable logging" FORCE)
set(FATAL_ASSERTS_ENABLED OFF CACHE STRING "Enable fatal assert" FORCE)

set(ENABLE_TRANSPOSITION_TABLE ON CACHE STRING "Enable fatal assert" FORCE)
set(ENABLE_LATE_MOVE_REDUCTION ON CACHE STRING "Enable late move reduction" FORCE)


set(PRECOMPILE_OPTIONS
    DEBUG_TRANSITION_TABLE
    DEBUG_LOGGING_ENABLED
    LOGGING_ENABLED
    FATAL_ASSERTS_ENABLED
    ENABLE_TRANSPOSITION_TABLE
    ENABLE_LATE_MOVE_REDUCTION
)