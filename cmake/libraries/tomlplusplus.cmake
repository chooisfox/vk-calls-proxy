set(CURRENT_LIBRARY_NAME tomlplusplus)

FetchContent_Declare(
    ${CURRENT_LIBRARY_NAME}
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
)

FetchContent_MakeAvailable(${CURRENT_LIBRARY_NAME})

list(APPEND PROJECT_LIBRARIES_LIST tomlplusplus::tomlplusplus)
