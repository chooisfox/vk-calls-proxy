set(CURRENT_LIBRARY_NAME stduuid)

FetchContent_Declare(
    ${CURRENT_LIBRARY_NAME}
    GIT_REPOSITORY https://github.com/mariusbancila/stduuid.git
    GIT_TAG        v1.2.3
    # Fix cmake_minimum_required version warning
    PATCH_COMMAND  sed -i "s/cmake_minimum_required(VERSION [0-9.]*)/cmake_minimum_required(VERSION 3.10)/g" CMakeLists.txt
)
FetchContent_MakeAvailable(${CURRENT_LIBRARY_NAME})

list(APPEND PROJECT_LIBRARIES_LIST ${CURRENT_LIBRARY_NAME})
