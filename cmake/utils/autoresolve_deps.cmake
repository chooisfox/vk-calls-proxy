function(autoresolve_dependencies)
    set(options)
    set(oneValueArgs OUTPUT_VAR)
    set(multiValueArgs SOURCE_LIST EXCLUDE_TARGETS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(FINAL_DEPS "")

    foreach(raw_item ${ARG_SOURCE_LIST})
        set(target_name "${raw_item}")

        if(TARGET "${target_name}")

            get_target_property(_aliased "${target_name}" ALIASED_TARGET)
            if(_aliased)
                set(target_name "${_aliased}")
            endif()

            if("${target_name}" IN_LIST ARG_EXCLUDE_TARGETS)
                continue()
            endif()

            get_target_property(_is_imported "${target_name}" IMPORTED)
            if(_is_imported)
                message(STATUS "Skipping system/imported target from install: ${target_name}")
                continue()
            endif()

            list(APPEND FINAL_DEPS "${target_name}")
        endif()
    endforeach()

    list(REMOVE_DUPLICATES FINAL_DEPS)
    set(${ARG_OUTPUT_VAR} ${FINAL_DEPS} PARENT_SCOPE)
endfunction()
