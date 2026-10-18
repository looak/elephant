# ElephantTuner sources, see tuner/CMakeLists.txt.

set(TUNER_SOURCE_ALL
## /...
    ${TUNER_SRC_DIR}/main.cpp
    ${TUNER_SRC_DIR}/dataset.cpp
    ${TUNER_SRC_DIR}/find_k.cpp
    ${TUNER_SRC_DIR}/fit.cpp
    ${TUNER_SRC_DIR}/quiet.cpp
    ${TUNER_SRC_DIR}/tuner_data.cpp
    ${TUNER_SRC_DIR}/write_tables.cpp

## /inc/...
    ${TUNER_INC_DIR}/dataset.hpp
    ${TUNER_INC_DIR}/find_k.hpp
    ${TUNER_INC_DIR}/fit.hpp
    ${TUNER_INC_DIR}/quiet.hpp
    ${TUNER_INC_DIR}/trace.hpp
    ${TUNER_INC_DIR}/tuner_data.hpp
    ${TUNER_INC_DIR}/write_tables.hpp
)
