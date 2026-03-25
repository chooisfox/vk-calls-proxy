include(cmake/utils/autoresolve_deps.cmake)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# [COMPILER OPTIONS]
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(
        -O3 -ffast-math -fvisibility=hidden
        -Wall -Wextra -pedantic
        -Wformat=2 -Wshadow -Wundef -Wcast-align
        -Wmissing-declarations -Woverloaded-virtual
    )
else()
    add_compile_options(-g -O0 -Wall -Wextra)
endif()

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

# [EXECUTABLE]
if(PROJECT_BUILD_INTERFACE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${PROJECT_MAIN_SRC_FILES})

    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME          "${PROJECT_APPLICATION_NAME}"
        MACOSX_BUNDLE_GUI_IDENTIFIER       "com.${PROJECT_ORGANIZATION_NAME}.${PROJECT_APPLICATION_NAME}"
        MACOSX_BUNDLE_VERSION              "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_INFO_STRING          "${PROJECT_APPLICATION_NAME} ${PROJECT_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT            "Copyright (c) 2026, ${PROJECT_ORGANIZATION_NAME}"
    )

    set(MAC_ICON_PATH "${PROJECT_SOURCE_DIR}/res/release/macos/icons/icon.icns")
    if(EXISTS "${MAC_ICON_PATH}")
        set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE "icon.icns")
        target_sources(${PROJECT_NAME} PRIVATE "${MAC_ICON_PATH}")
        set_source_files_properties("${MAC_ICON_PATH}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    endif()
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
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        COMPONENT Runtime
    )

    set(MAC_INSTALL_RES_DIR "${PROJECT_SOURCE_DIR}/res/install/macos/")
    if(EXISTS "${MAC_INSTALL_RES_DIR}")
        install(DIRECTORY "${MAC_INSTALL_RES_DIR}"
            DESTINATION "${PROJECT_APPLICATION_NAME}.app/Contents/Resources"
            COMPONENT Runtime
        )
    endif()

    if(TARGET Qt6::qmake)
        get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
        find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")
    endif()

    if(MACDEPLOYQT_EXECUTABLE)
        install(CODE "
            message(STATUS \"Running macdeployqt on ${PROJECT_APPLICATION_NAME}.app...\")
            execute_process(
                COMMAND \"${MACDEPLOYQT_EXECUTABLE}\" \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_APPLICATION_NAME}.app\" -always-overwrite
                RESULT_VARIABLE _deploy_result
            )
            if(NOT _deploy_result EQUAL 0)
                message(WARNING \"macdeployqt finished with errors! Check the logs.\")
            endif()
        " COMPONENT Runtime)
    else()
        message(WARNING "macdeployqt not found! Your App Bundle will lack required Qt6 frameworks.")
    endif()
endif()

# [CPACK]
set(CPACK_GENERATOR "DragNDrop")
set(CPACK_PACKAGE_NAME "${PROJECT_APPLICATION_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")

if(CMAKE_OSX_ARCHITECTURES MATCHES ";")
    set(MAC_ARCH "universal")
else()
    set(MAC_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-macOS-${MAC_ARCH}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 1)

if(PROJECT_BUILD_INTERFACE)
    set(CPACK_COMPONENTS_ALL Runtime Libraries)
else()
    set(CPACK_COMPONENTS_ALL Libraries)
endif()

set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${PROJECT_NAME};Runtime;/")

include(CPack)
