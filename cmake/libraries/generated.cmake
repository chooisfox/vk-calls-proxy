include(cmake/utils/generate_resources.cmake)

set(CURRENT_LIBRARY_NAME generated_resources)
set(RES_INPUT_DIR "${PROJECT_MAIN_RES_DIR}/defaults")

set(RES_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/common/generated")

add_resource_generation(
    TARGET ${CURRENT_LIBRARY_NAME}
    INPUT_DIR ${RES_INPUT_DIR}
    OUTPUT_DIR ${RES_OUTPUT_DIR}
)

add_library(CHOOI::${CURRENT_LIBRARY_NAME} ALIAS ${CURRENT_LIBRARY_NAME})

list(APPEND PROJECT_LIBRARIES_LIST CHOOI::${CURRENT_LIBRARY_NAME})
