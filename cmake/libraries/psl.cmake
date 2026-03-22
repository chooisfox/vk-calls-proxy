include(ExternalProject)

set(CURRENT_LIBRARY_NAME libpsl)

FetchContent_Declare(
    ${CURRENT_LIBRARY_NAME}
    GIT_REPOSITORY https://github.com/rockdaboot/libpsl.git
    GIT_TAG        0.21.5
)
FetchContent_MakeAvailable(${CURRENT_LIBRARY_NAME})

set(PSL_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/psl_install)
set(PSL_SOURCE_DIR ${${CURRENT_LIBRARY_NAME}_SOURCE_DIR})

ExternalProject_Add(
    libpsl_external
    SOURCE_DIR        ${PSL_SOURCE_DIR}
    GIT_REPOSITORY    ""
    GIT_TAG           ""

    CONFIGURE_COMMAND meson setup <SOURCE_DIR> <BINARY_DIR> --prefix=<INSTALL_DIR> --default-library=static
    BUILD_COMMAND     ninja -C <BINARY_DIR>
    INSTALL_COMMAND   ninja -C <BINARY_DIR> install

    INSTALL_DIR       ${PSL_INSTALL_DIR}
)

add_library(PSL::libpsl STATIC IMPORTED GLOBAL)

set_property(TARGET PSL::libpsl PROPERTY
             IMPORTED_LOCATION ${PSL_INSTALL_DIR}/lib/libpsl.a)
set_property(TARGET PSL::libpsl PROPERTY
             INTERFACE_INCLUDE_DIRECTORIES ${PSL_SOURCE_DIR}/include)

add_dependencies(PSL::libpsl libpsl_external)

list(APPEND PROJECT_LIBRARIES_LIST PSL::libpsl)
