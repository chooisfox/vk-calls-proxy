
if(NOT DEFINED INPUT_FILE OR NOT DEFINED OUTPUT_FILE OR NOT DEFINED VAR_NAME)
    message(FATAL_ERROR "Missing arguments: INPUT_FILE, OUTPUT_FILE, or VAR_NAME")
endif()

file(READ "${INPUT_FILE}" FILE_CONTENT)

set(RAW_DELIM "EMBED_DELIM")
set(GUARD_NAME "GENERATED_RESOURCE_${VAR_NAME}_HPP")

set(CPP_CONTENT
"/**
 * AUTOMATICALLY GENERATED FILE - DO NOT EDIT
 * Source: ${INPUT_FILE}
 */
#ifndef ${GUARD_NAME}
#define ${GUARD_NAME}

#include <string_view>

namespace RESOURCES {

constexpr std::string_view ${VAR_NAME} = R\"${RAW_DELIM}(
${FILE_CONTENT}
)${RAW_DELIM}\";

} // namespace RESOURCES
#endif // ${GUARD_NAME}
")

file(WRITE "${OUTPUT_FILE}" "${CPP_CONTENT}")
