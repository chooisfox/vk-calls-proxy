set(CURRENT_LIBRARY_NAME curl)

FetchContent_Declare(
    ${CURRENT_LIBRARY_NAME}
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG        curl-8_15_0
)
FetchContent_MakeAvailable(${CURRENT_LIBRARY_NAME})

list(APPEND PROJECT_LIBRARIES_LIST CURL::libcurl)
