find_package(PkgConfig REQUIRED)

pkg_check_modules(LibSecret REQUIRED IMPORTED_TARGET libsecret-1)

list(APPEND PROJECT_LIBRARIES_LIST PkgConfig::LibSecret)
list(APPEND PROJECT_INCLUDE_DIRS ${LibSecret_INCLUDE_DIRS})
