include(cmake/utils/get_linux_kernel.cmake)
include(cmake/utils/autoresolve_deps.cmake)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(
        -fvisibility=hidden
        -pedantic
        -Wall
        -Wextra
        -Wcast-align
        -Wcast-qual
        -Wctor-dtor-privacy
        -Wformat=2
        -Winit-self
        -Wmissing-declarations
        -Wmissing-include-dirs
        -Woverloaded-virtual
        -Wredundant-decls
        -Wshadow
        -Wsign-promo
        -Wswitch-default
        -Wundef
        -Wno-unused-variable
        -Wno-error=redundant-decls
        -Wno-uninitialized
        -Wno-strict-overflow
        -O3
        -ffast-math
    )
else()
    add_compile_options(
        -Wall
        -Wextra
    )
endif()

list(APPEND PROJECT_LIBRARIES_LIST pthread)

if(ENABLE_ASAN)
    list(APPEND PROJECT_LIBRARIES_LIST asan)
    list(APPEND PROJECT_LIBRARIES_LIST ubsan)
    add_compile_options(
        -fsanitize=address
        -fsanitize=undefined
        -static-libasan
    )
endif()

# [INSTALL RESOURCES DIRECTORY]
set(PROJECT_INSTALL_DIR "${CMAKE_SOURCE_DIR}/res/install/linux/")

# [EXECUTABLE]
if(PROJECT_BUILD_INTERFACE)
    add_executable(${PROJECT_NAME} ${PROJECT_MAIN_SRC_FILES})
endif()

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

install(TARGETS ${INTERNAL_LIBS} ${DEPENDENCY_LIBS}
    EXPORT ${PROJECT_NAME}Targets
    ARCHIVE       DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY       DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME       DESTINATION "${CMAKE_INSTALL_BINDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
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

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    COMPONENT Libraries
)

if(PROJECT_BUILD_INTERFACE)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        message(STATUS "No CMAKE_INSTALL_PREFIX provided. Configuring a standard system-wide installation.")

        install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT Runtime
        )

        configure_file(
            "${PROJECT_SOURCE_DIR}/res/release/linux/desktop/app.desktop.in"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.desktop"
            @ONLY
        )

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
        message(STATUS "User-defined CMAKE_INSTALL_PREFIX detected: ${CMAKE_INSTALL_PREFIX}")
        message(STATUS "Configuring a relocatable installation with a launcher.")

        install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION "bin"
            COMPONENT Runtime
        )

        configure_file(
          "${PROJECT_SOURCE_DIR}/res/release/linux/launcher/launcher.sh.in"
          "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-launcher.sh"
          @ONLY
        )

        install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-launcher.sh"
            DESTINATION "bin"
            RENAME ${PROJECT_NAME}
            COMPONENT Runtime
        )

        configure_file(
            "${PROJECT_SOURCE_DIR}/res/release/linux/desktop/app.desktop.in"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.desktop"
            @ONLY
        )

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
          set(installed_executable \"${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}\")
          message(STATUS \"--- Analyzing runtime dependencies for: \${installed_executable}\")
          file(GET_RUNTIME_DEPENDENCIES
              EXECUTABLES \${installed_executable}
              RESOLVED_DEPENDENCIES_VAR resolved_deps
          )
          message(STATUS \"--- Copying dependencies to ${CMAKE_INSTALL_PREFIX}/lib\")
          file(INSTALL DESTINATION \"${CMAKE_INSTALL_PREFIX}/lib\" TYPE FILE FILES \${resolved_deps})
        " COMPONENT Runtime)
    endif()
endif()

set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-Linux-x86_64")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 1)

if(PROJECT_BUILD_INTERFACE)
    set(CPACK_COMPONENTS_ALL Runtime Libraries)
else()
    set(CPACK_COMPONENTS_ALL Libraries)
endif()

set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${PROJECT_NAME};Runtime;/")

include(CPack)
