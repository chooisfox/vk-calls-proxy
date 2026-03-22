function(add_resource_generation)
    set(options)
    set(oneValueArgs TARGET INPUT_DIR OUTPUT_DIR)
    set(multiValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_TARGET OR NOT ARG_INPUT_DIR OR NOT ARG_OUTPUT_DIR)
        message(FATAL_ERROR "Usage: add_resource_generation(TARGET <name> INPUT_DIR <path> OUTPUT_DIR <path>)")
    endif()

    file(MAKE_DIRECTORY ${ARG_OUTPUT_DIR})
    file(GLOB_RECURSE RESOURCE_FILES "${ARG_INPUT_DIR}/*")

    set(GENERATED_FILES_LIST "")
    set(GENERATOR_TARGET_NAME "${ARG_TARGET}_generator")

    foreach(FILE_PATH ${RESOURCE_FILES})
        get_filename_component(FILE_NAME ${FILE_PATH} NAME)
        get_filename_component(FILE_NAME_WE ${FILE_PATH} NAME_WE)
        get_filename_component(FILE_EXT ${FILE_PATH} EXT)

        string(REPLACE "." "_" VAR_NAME ${FILE_NAME})
        string(REPLACE "-" "_" VAR_NAME ${VAR_NAME})
        string(TOUPPER ${VAR_NAME} VAR_NAME)

        set(OUTPUT_FILE "${ARG_OUTPUT_DIR}/${FILE_NAME_WE}.hpp")

        if("${FILE_EXT}" MATCHES "\\.(toml|js)$")
            add_custom_command(
                OUTPUT "${OUTPUT_FILE}"
                COMMAND ${CMAKE_COMMAND}
                    -DINPUT_FILE=${FILE_PATH}
                    -DOUTPUT_FILE=${OUTPUT_FILE}
                    -DVAR_NAME=${VAR_NAME}
                    -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/generation_rules/embed_as_string.cmake"
                DEPENDS "${FILE_PATH}" "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/generation_rules/embed_as_string.cmake"
                COMMENT "Generating C++ header for ${FILE_NAME}"
                VERBATIM
            )
            list(APPEND GENERATED_FILES_LIST "${OUTPUT_FILE}")
        endif()
    endforeach()

    add_custom_target(${GENERATOR_TARGET_NAME} DEPENDS ${GENERATED_FILES_LIST})

    add_library(${ARG_TARGET} INTERFACE)

    add_dependencies(${ARG_TARGET} ${GENERATOR_TARGET_NAME})

    target_include_directories(${ARG_TARGET} INTERFACE
        $<BUILD_INTERFACE:${ARG_OUTPUT_DIR}/..>
        $<INSTALL_INTERFACE:include/generated>
    )

    # target_sources(${ARG_TARGET} INTERFACE
    #     $<BUILD_INTERFACE:${GENERATED_FILES_LIST}>
    # )

    install(FILES ${GENERATED_FILES_LIST}
        DESTINATION "include/generated/${ARG_TARGET}"
        COMPONENT Libraries
    )
endfunction()
