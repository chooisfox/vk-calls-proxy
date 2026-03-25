include(FetchContent)
include(ExternalProject)
include(cmake/utils/list_all_subdirectories.cmake)

if(PROJECT_BUILD_SHARED)
    set(BUILD_SHARED_LIBS ON)
else()
    set(BUILD_SHARED_LIBS OFF)
endif()

message(STATUS "CXX compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Building Interface: ${PROJECT_BUILD_INTERFACE}")
message(STATUS "Building Shared Libs: ${PROJECT_BUILD_SHARED}")

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No CMAKE_BUILD_TYPE selected, defaulting to Release.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build as release" FORCE)
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

# [SOURCE DIRECTORIES]
set(PROJECT_MAIN_SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(PROJECT_MAIN_RES_DIR "${CMAKE_SOURCE_DIR}/res")

if(PROJECT_BUILD_INTERFACE)
    file(GLOB PROJECT_MAIN_SRC_FILES CONFIGURE_DEPENDS
        "${PROJECT_MAIN_SRC_DIR}/*.hpp"
        "${PROJECT_MAIN_SRC_DIR}/*.cpp"
    )
    source_group("Main" FILES ${PROJECT_MAIN_SRC_FILES})
endif()

list(APPEND PROJECT_INCLUDE_DIRS ${PROJECT_MAIN_SRC_DIR})

# [VARIABLES]
set(PROJECT_INCLUDE_DIRS)
set(PROJECT_LIBRARIES_LIST)
set(PROJECT_DIRECTORIES_LIST)

# [LIBRARIES]
include(cmake/libraries/cxxopts.cmake)
include(cmake/libraries/qt6.cmake)
include(cmake/libraries/generated.cmake)
include(cmake/libraries/fmt.cmake)
include(cmake/libraries/spdlog.cmake)
include(cmake/libraries/tomlplusplus.cmake)
include(cmake/libraries/api.cmake)
include(cmake/libraries/common.cmake)
include(cmake/libraries/utils.cmake)

if(PROJECT_BUILD_INTERFACE)
    include(cmake/libraries/server.cmake)
    include(cmake/libraries/app.cmake)
endif()

if(APPLE)
    include(cmake/platform/macos_builder.cmake)
elseif(LINUX)
    include(cmake/platform/linux_builder.cmake)
elseif(WIN32)
    include(cmake/platform/windows_builder.cmake)
else()
    message(FATAL_ERROR "Unsupported OS: ${CMAKE_SYSTEM_NAME}")
endif()

# [TARGET LINKING]
if(PROJECT_BUILD_INTERFACE)
    target_include_directories(${PROJECT_NAME} PRIVATE   ${PROJECT_INCLUDE_DIRS})
    target_include_directories(${PROJECT_NAME} PRIVATE   ${PROJECT_DIRECTORIES_LIST})
    target_link_directories(${PROJECT_NAME}    PRIVATE   ${PROJECT_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME}      PRIVATE   ${PROJECT_LIBRARIES_LIST})

    if(CMAKE_SYSTEM_TILING)
        target_compile_definitions(${PROJECT_NAME} PUBLIC SYSTEM_IS_TILING="${CMAKE_SYSTEM_TILING}")
    endif()
endif()

include(cmake/utils/postbuild_scripts.cmake)
