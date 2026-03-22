execute_process(
    COMMAND cat /etc/os-release
    COMMAND grep -oP "^ID=(\"|)\\K[a-zA-Z0-9]*"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    RESULT_VARIABLE OS_RELEASE_RESULT
    OUTPUT_VARIABLE CMAKE_SYSTEM_ID
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(REPLACE "\"" "" CMAKE_SYSTEM_ID ${CMAKE_SYSTEM_ID})
string(REPLACE "'" "" CMAKE_SYSTEM_ID ${CMAKE_SYSTEM_ID})

if(OS_RELEASE_RESULT OR NOT CMAKE_SYSTEM_ID)
    message(WARNING "Unknown kernel ID, /etc/os-release not found. Attempting to use lsb_release.")

    find_program(LSB_RELEASE_EXEC lsb_release)

    if(LSB_RELEASE_EXEC)
        execute_process(COMMAND ${LSB_RELEASE_EXEC} -is OUTPUT_VARIABLE CMAKE_SYSTEM_ID OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        message(WARNING "Unknown kernel ID, lsb_release not found. Distribution ID treated as unknown.")
        set(CMAKE_SYSTEM_ID "unknown")
    endif()
endif()

execute_process(
    COMMAND bash -c "echo $XDG_CURRENT_DESKTOP $DESKTOP_SESSION $XDG_SESSION_DESKTOP $GDMSESSION"
    OUTPUT_VARIABLE WM_ENV
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if("${WM_ENV}" MATCHES ".*bspwm.*|.*i3.*|.*awesome.*|.*dwm.*|.*xmonad.*|.*herbstluftwm.*|.*qtile.*|.*spectrwm.*|.*Hyprland.*|.*sway.*")
    message(STATUS "Detected a tiling window manager")
    set(CMAKE_SYSTEM_TILING 1)
else()
    set(CMAKE_SYSTEM_TILING 0)
endif()

message(STATUS "Kernel Name:    ${CMAKE_SYSTEM_NAME}")
message(STATUS "Kernel Version: ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Kernel ID:      ${CMAKE_SYSTEM_ID}")
message(STATUS "Processor:      ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Tiling WM:      ${CMAKE_SYSTEM_TILING}")

string(TOLOWER "${CMAKE_SYSTEM_ID}" CMAKE_SYSTEM_ID)

include(cmake/utils/fun/linux_distro_ramble.cmake)
