include(cmake/utils/get_windows_kernel.cmake)
include(cmake/utils/autoresolve_deps.cmake)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# [COMPILER OPTIONS]
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        add_compile_options(
            /O2 /Oi /Ot /Gy /Gw /W4 /permissive- /volatile:iso /EHsc
            /Zc:forScope /Zc:inline /Zc:rvalueCast /Zc:strictStrings
            /Zc:throwingNew /Zc:wchar_t
        )
        add_link_options(/OPT:REF /OPT:ICF)
    elseif(MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(
            -O3 -fomit-frame-pointer -funroll-loops
            -Wall -Wextra -fno-strict-aliasing -fno-common
            -D__USE_MINGW_ANSI_STDIO=1
        )
    endif()
else()
    if(MSVC)
        add_compile_options(/W4 /MDd /Zi /Ob0 /Od /RTC1)
    elseif(MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wall -Wextra -g -O0 -DDEBUG)
    endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_error)
    if(_ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

# [EXECUTABLE]
if(PROJECT_BUILD_INTERFACE)
    set(WIN_ICON_PATH "${PROJECT_SOURCE_DIR}/res/release/windows/icon.rc")
    if(EXISTS "${WIN_ICON_PATH}")
        list(APPEND PROJECT_MAIN_SRC_FILES "${WIN_ICON_PATH}")
    endif()

    add_executable(${PROJECT_NAME} WIN32 ${PROJECT_MAIN_SRC_FILES})
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
    LIBRARY       DESTINATION "${CMAKE_INSTALL_BINDIR}"
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
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT Runtime
    )

    set(WIN_INSTALL_RES_DIR "${PROJECT_SOURCE_DIR}/res/install/windows/")
    if(EXISTS "${WIN_INSTALL_RES_DIR}")
        install(DIRECTORY "${WIN_INSTALL_RES_DIR}"
            DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT Runtime
        )
    endif()

    if(TARGET Qt6::qmake)
        get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    endif()

    if(WINDEPLOYQT_EXECUTABLE)
        install(CODE "
            message(STATUS \"Running windeployqt on ${PROJECT_NAME}.exe...\")
            execute_process(
                COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                        --no-compiler-runtime
                        --no-opengl-sw
                        \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME}.exe\"
                RESULT_VARIABLE _deploy_result
            )
            if(NOT _deploy_result EQUAL 0)
                message(WARNING \"windeployqt finished with errors! Check the logs.\")
            endif()
        " COMPONENT Runtime)
    else()
        message(WARNING "windeployqt not found. DLL deployment will be incomplete.")
    endif()
endif()

# [CPACK]
set(CPACK_GENERATOR "NSIS")
set(CPACK_NSIS_COMPRESSION_TYPE "lzma")
set(CPACK_PACKAGE_NAME "${PROJECT_APPLICATION_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "${PROJECT_ORGANIZATION_NAME}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_ORGANIZATION_NAME}\\${PROJECT_APPLICATION_NAME}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-Windows-${CMAKE_SYSTEM_PROCESSOR}")

if(PROJECT_BUILD_INTERFACE)
    set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_APPLICATION_NAME}")

    set(CPACK_PACKAGE_EXECUTABLES "${PROJECT_NAME}" "${PROJECT_APPLICATION_NAME}")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\${PROJECT_NAME}.exe")
    set(CPACK_NSIS_URL_INFO_ABOUT "${PROJECT_ORGANIZATION_WEBSITE}")

    set(NSIS_ICON_PATH "${PROJECT_SOURCE_DIR}/res/release/windows/installer_icon.ico")
    if(EXISTS "${NSIS_ICON_PATH}")
        file(TO_NATIVE_PATH "${NSIS_ICON_PATH}" NATIVE_NSIS_ICON_PATH)
        string(REPLACE "\\" "\\\\" NATIVE_NSIS_ICON_PATH "${NATIVE_NSIS_ICON_PATH}")
        set(CPACK_NSIS_MUI_ICON "${NATIVE_NSIS_ICON_PATH}")
        set(CPACK_NSIS_MUI_UNIICON "${NATIVE_NSIS_ICON_PATH}")
    endif()

    set(CPACK_COMPONENTS_ALL Runtime Libraries)
else()
    set(CPACK_COMPONENTS_ALL Libraries)
endif()

include(CPack)
