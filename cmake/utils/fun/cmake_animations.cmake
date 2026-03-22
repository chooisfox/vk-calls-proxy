include(cmake/utils/fun/ansi.cmake)

set(LAST_FRAME_WIDTH 0 CACHE INTERNAL "Last known terminal width")

function(get_terminal_width return_var)
    find_program(STTY_COMMAND stty)
    if(STTY_COMMAND)
        execute_process(
            COMMAND ${STTY_COMMAND} size
            OUTPUT_VARIABLE STTY_OUTPUT
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(REGEX MATCH "[0-9]+$" TERMINAL_WIDTH "${STTY_OUTPUT}")
    endif()

    if(NOT TERMINAL_WIDTH)
        execute_process(
            COMMAND bash -c "echo -en \"\\E[999C\\E[6n\" > /dev/tty &&
                            read -sdR CURPOS < /dev/tty &&
                            CURPOS=\${CURPOS#*[} &&
                            COL=\${CURPOS#*;} &&
                            echo \$COL"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            RESULT_VARIABLE RESULT
            OUTPUT_VARIABLE OUTPUT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        message(" ")
    endif()

    if(NOT TERMINAL_WIDTH)
        message(WARNING "Could not determine terminal width via stty. Falling back to 80 columns.")
        set(TERMINAL_WIDTH 80)
    endif()

    set(${return_var} ${TERMINAL_WIDTH} PARENT_SCOPE)
endfunction()

function(calculate_line_width contents return_var)
    execute_process(
        COMMAND bash -c "
        echo \"${contents}\" | awk '{
            gsub(/\\x1b\[[0-9;]*m/, \"\");
            if (length > max_length) max_length = length
        } END { print max_length }'
        "
        OUTPUT_VARIABLE WIDTH
        ERROR_QUIET
    )
    set(${return_var} "${WIDTH}" PARENT_SCOPE)
endfunction()

function(calculate_left_offset terminal_width line_width return_var)
    math(EXPR OFFSET "(${terminal_width} - ${line_width}) / 2")
    if(OFFSET LESS 0)
        set(OFFSET 0)
    endif()
    set(${return_var} ${OFFSET} PARENT_SCOPE)
endfunction()

function(draw_animation_frame ansi_file animation_dir)
    get_terminal_width(TERMINAL_WIDTH)

    if(NOT LAST_FRAME_WIDTH EQUAL TERMINAL_WIDTH)
        set(LAST_FRAME_WIDTH ${TERMINAL_WIDTH} PARENT_SCOPE)
        message("${ANSI_ERASE_FULL_DISPLAY}")
    endif()

    message("${ANSI_HOME}${ANSI_DISPLAY_MM_MODE}")

    get_filename_component(FRAME_BASENAME ${ansi_file} NAME_WE)
    string(REPLACE "_" ";" FRAME_PARTS "${FRAME_BASENAME}")
    list(GET FRAME_PARTS -1 FRAME_NUMBER)
    set(SPECIFIC_TITLE_FILE "${animation_dir}/title_${FRAME_NUMBER}.txt")
    set(GENERIC_TITLE_FILE "${animation_dir}/title.txt")

    set(TEXT_TOP "")
    if(EXISTS "${SPECIFIC_TITLE_FILE}")
        file(READ "${SPECIFIC_TITLE_FILE}" TEXT_TOP)
    elseif(EXISTS "${GENERIC_TITLE_FILE}")
        file(READ "${GENERIC_TITLE_FILE}" TEXT_TOP)
    endif()

    if(TEXT_TOP)
        string(STRIP "${TEXT_TOP}" TEXT_TOP)
        calculate_line_width("${TEXT_TOP}" TEXT_TOP_WIDTH)
        calculate_left_offset(${TERMINAL_WIDTH} ${TEXT_TOP_WIDTH} TEXT_TOP_OFFSET)
        ansi_cursor_right(${TEXT_TOP_OFFSET} MOVE_RIGHT)
        message("${MOVE_RIGHT}${TEXT_TOP}${ANSI_ERASE_END_LINE}")
    endif()

    message("")

    file(READ ${ansi_file} CONTENTS)
    execute_process(
        COMMAND bash -c "
        while IFS= read -r line; do
            echo -e \"\$line\"
        done <<< \"${CONTENTS}\""
        OUTPUT_VARIABLE CONTENTS
        ERROR_QUIET
    )

    calculate_line_width("${CONTENTS}" IMAGE_WIDTH)
    calculate_left_offset(${TERMINAL_WIDTH} ${IMAGE_WIDTH} IMAGE_OFFSET)

    execute_process(
        COMMAND bash -c "
        while IFS= read -r line; do
            echo -e \"${ANSI_ESC}[${IMAGE_OFFSET}C\$line\"
        done <<< \"${CONTENTS}\""
        OUTPUT_VARIABLE CONTENTS
        ERROR_QUIET
    )
    message("${CONTENTS}")
endfunction()

function(draw_animation repeat_count delay_seconds subdir)
    set(BASE_ANIMATION_DIR "${CMAKE_SOURCE_DIR}/cmake/utils/fun/animation")

    if(subdir)
        set(FINAL_ANIMATION_DIR "${BASE_ANIMATION_DIR}/${subdir}")
    else()
        set(FINAL_ANIMATION_DIR "${BASE_ANIMATION_DIR}")
    endif()

    file(GLOB ANSI_FILES RELATIVE "${FINAL_ANIMATION_DIR}" "${FINAL_ANIMATION_DIR}/*.ansi")
    list(SORT ANSI_FILES)

    if(NOT ANSI_FILES)
        message(WARNING "No .ansi files found in ${FINAL_ANIMATION_DIR}. Cannot play animation.")
        return()
    endif()

    message("${ANSI_ERASE_FULL_DISPLAY}")

    message("${ANSI_SET_HIDDEN_MODE}${ANSI_ERASE_FULL_DISPLAY}")

    math(EXPR repeat_count "${repeat_count} - 1")
    while(repeat_count GREATER_EQUAL 0)
        foreach(ansi_filename IN LISTS ANSI_FILES)
            set(full_path "${FINAL_ANIMATION_DIR}/${ansi_filename}")
            draw_animation_frame(${full_path} ${FINAL_ANIMATION_DIR})
            execute_process(COMMAND ${CMAKE_COMMAND} -E sleep ${delay_seconds})
        endforeach()
        math(EXPR repeat_count "${repeat_count} - 1")
    endwhile()

    message("${ANSI_ERASE_FULL_DISPLAY}${ANSI_HOME}${ANSI_SET_VISIBLE_MODE}")
endfunction()
