cmake_minimum_required(VERSION 3.21.1)

option(CMAKE_DISABLE_SOURCE_CHANGES  "Disable source changes" ON)
option(CMAKE_DISABLE_IN_SOURCE_BUILD "Disable in-source build" ON)

include(cmake/utils/build_protection.cmake)

project(vk-call-proxy
    VERSION     1.0.0
    DESCRIPTION "High-Performance C++ VK Call Proxy"
    LANGUAGES   C CXX
)

if("${PROJECT_NAME}" MATCHES " ")
    message(FATAL_ERROR "Project name '${PROJECT_NAME}' contains spaces. Please remove them.")
endif()

set(PROJECT_APPLICATION_NAME     "VK WebRTC Call Proxy"          CACHE STRING "Application name")
set(PROJECT_ORGANIZATION_NAME    "Blueline-Software"             CACHE STRING "Organization's name")
set(PROJECT_ORGANIZATION_WEBSITE "redline-software.moscow"      CACHE STRING "Organization's website")
set(PROJECT_DEVELOPER_NAME       "chooisfox"                     CACHE STRING "Developer's name")
set(PROJECT_DEVELOPER_EMAIL      "admin@redline-software.moscow" CACHE STRING "Developer's email")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS   ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

option(ENABLE_UPX_COMPRESS "Enable UPX compression"      OFF)
option(ENABLE_STRIP        "Enable binary stripping"     OFF)
option(ENABLE_CLANG_TIDY   "Enable Clang-Tidy"           OFF)
option(ENABLE_CPPCHECK     "Enable Cppcheck"             OFF)
option(ENABLE_ASAN         "Enable AddressSanitizer"     OFF)
option(ENABLE_ANIMATION    "Enable precompile animation" ON)

option(PROJECT_BUILD_INTERFACE "Build project interface"     ON)
option(PROJECT_BUILD_SHARED    "Build shared libraries"      OFF)

include(cmake/platform/generic_builder.cmake)
