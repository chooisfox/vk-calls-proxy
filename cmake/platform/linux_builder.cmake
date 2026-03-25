include(cmake/utils/get_linux_kernel.cmake)
include(cmake/utils/autoresolve_deps.cmake)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# [COMPILER OPTIONS]
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(
        -O3 -ffast-math -fvisibility=hidden
        -Wall -Wextra -pedantic
        -Wformat=2 -Wshadow -Wundef -Wcast-align
        -Wmissing-declarations -Woverloaded-virtual
    )
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_error)
    if(_ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
else()
    add_compile_options(-g -O0 -Wall -Wextra)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND PROJECT_LIBRARIES_LIST Threads::Threads)

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

if(PROJECT_BUILD_INTERFACE)
    add_executable(${PROJECT_NAME} ${PROJECT_MAIN_SRC_FILES})
endif()

# [DEPENDENCIES]
set(INTERNAL_LIBS common api utils generated_resources)
if(PROJECT_BUILD_INTERFACE)
    list(APPEND INTERNAL_LIBS server app)
endif()

autoresolve_dependencies(
    SOURCE_LIST     "${PROJECT_LIBRARIES_LIST}"
    EXCLUDE_TARGETS "${INTERNAL_LIBS}"
    OUTPUT_VAR      DEPENDENCY_LIBS
)
message(STATUS "Auto-resolved exportable dependencies: ${DEPENDENCY_LIBS}")

# [INSTALL]
install(TARGETS ${INTERNAL_LIBS} ${DEPENDENCY_LIBS}
    EXPORT ${PROJECT_NAME}Targets
    ARCHIVE       DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY       DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME       DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT Libraries
)

install(DIRECTORY "${PROJECT_MAIN_SRC_DIR}/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT Libraries
    FILES_MATCHING PATTERN "*.hpp"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT ${PROJECT_NAME}Targets
    FILE "${PROJECT_NAME}Targets.cmake"
    NAMESPACE xui::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    COMPONENT Libraries
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    COMPONENT Libraries
)

# [DEPLOY]
if(PROJECT_BUILD_INTERFACE)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        message(STATUS "Default CMAKE_INSTALL_PREFIX. Configuring a system-wide installation.")

        install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT Runtime
        )

        configure_file("${PROJECT_SOURCE_DIR}/res/release/linux/desktop/app.desktop.in"
                       "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.desktop" @ONLY)

        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.desktop"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
            COMPONENT Runtime
        )
        install(FILES "${PROJECT_SOURCE_DIR}/res/release/linux/icons/icon.png"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps"
            RENAME "${PROJECT_NAME}.png"
            COMPONENT Runtime
        )
    else()
        message(STATUS "Custom CMAKE_INSTALL_PREFIX detected. Configuring a relocatable installation.")

        install(TARGETS ${PROJECT_NAME}
                RUNTIME DESTINATION "libexec/${PROJECT_NAME}"
                COMPONENT Runtime
            )

        configure_file("${PROJECT_SOURCE_DIR}/res/release/linux/launcher/launcher.sh.in"
                           "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-launcher.sh" @ONLY)

        install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-launcher.sh"
                DESTINATION "bin"
                RENAME "${PROJECT_NAME}"
                COMPONENT Runtime
            )

        configure_file("${PROJECT_SOURCE_DIR}/res/release/linux/desktop/app.desktop.in"
                       "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.desktop" @ONLY)

        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.desktop"
            DESTINATION "share/applications"
            COMPONENT Runtime
        )
        install(FILES "${PROJECT_SOURCE_DIR}/res/release/linux/icons/icon.png"
            DESTINATION "share/icons/hicolor/256x256/apps"
            RENAME "${PROJECT_NAME}.png"
            COMPONENT Runtime
        )

        install(CODE "
                    set(installed_executable \"\${CMAKE_INSTALL_PREFIX}/libexec/${PROJECT_NAME}/${PROJECT_NAME}\")
                    message(STATUS \"--- Analyzing runtime dependencies for: \${installed_executable}\")

                    file(GET_RUNTIME_DEPENDENCIES
                        EXECUTABLES \${installed_executable}
                        RESOLVED_DEPENDENCIES_VAR resolved_deps
                        UNRESOLVED_DEPENDENCIES_VAR unresolved_deps
                        POST_EXCLUDE_REGEXES
                            \".*libc\\\\.so.*\"
                            \".*libm\\\\.so.*\"
                            \".*libpthread\\\\.so.*\"
                            \".*libdl\\\\.so.*\"
                            \".*libstdc\\\\+\\\\+\\\\.so.*\"
                            \".*libgcc_s\\\\.so.*\"
                    )

                    if(resolved_deps)
                        message(STATUS \"--- Copying non-system dependencies to \${CMAKE_INSTALL_PREFIX}/lib\")
                        file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/lib\" TYPE FILE FILES \${resolved_deps})
                    endif()
                " COMPONENT Runtime)
    endif()
endif()

# [CPACK]
set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_NAME "${PROJECT_APPLICATION_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "${PROJECT_ORGANIZATION_NAME}")

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-Linux-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 1)

if(PROJECT_BUILD_INTERFACE)
    set(CPACK_COMPONENTS_ALL Runtime Libraries)
else()
    set(CPACK_COMPONENTS_ALL Libraries)
endif()

set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${PROJECT_NAME};Runtime;/")

include(CPack)
