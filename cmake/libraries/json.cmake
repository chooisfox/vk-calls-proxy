set(CURRENT_LIBRARY_NAME nlohmann_json)

FetchContent_Declare(
    ${CURRENT_LIBRARY_NAME}
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.12.0
)
FetchContent_MakeAvailable(${CURRENT_LIBRARY_NAME})

list(APPEND PROJECT_LIBRARIES_LIST nlohmann_json::nlohmann_json)
