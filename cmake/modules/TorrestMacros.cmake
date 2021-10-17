# Various helper function and macros for building torrest
include(FeatureSummary)

# macro for issuing option() and add_feature_info() in a single call.
# Synopsis:
# feature_option(<option_and_feature_name> <option_and_feature_description> <default_option_value>)
macro(feature_option _name _description _default)
    option(${_name} "${_description}" ${_default})
    add_feature_info(${_name} ${_name} "${_description}")
endmacro()

# function to add a simple build option which controls compile definition(s) for a target.
# Synopsis:
# target_optional_compile_definitions(<target> [FEATURE]
#   NAME <name> DESCRIPTION <description> DEFAULT <default_value>
#   [ENABLED [enabled_compile_definitions...]]
#   [DISABLED [disabled_compile_defnitions...]]
# )
# NAME, DESCRIPTION and DEFAULT are passed to option() call
# if FEATURE is given, they are passed to add_feature_info()
# ENABLED lists compile definitions that will be set on <target> when option is enabled,
# DISABLED lists definitions that will be set otherwise
function(target_optional_compile_definitions _target _scope)
    set(options FEATURE)
    set(oneValueArgs NAME DESCRIPTION DEFAULT)
    set(multiValueArgs ENABLED DISABLED)
    cmake_parse_arguments(TOCD "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    option(${TOCD_NAME} "${TOCD_DESCRIPTION}" ${TOCD_DEFAULT})

    if (${${TOCD_NAME}})
        target_compile_definitions(${_target} ${_scope} ${TOCD_ENABLED})
    else()
        target_compile_definitions(${_target} ${_scope} ${TOCD_DISABLED})
    endif()

    if (${TOCD_FEATURE})
        add_feature_info(${TOCD_NAME} ${TOCD_NAME} "${TOCD_DESCRIPTION}")
    endif()
endfunction()

# function for parsing the version from a header file
function(read_version _verFile _outVarVersion)
    file(STRINGS ${_verFile} verLine REGEX "VERSION[ \t]+\"")
    string(REGEX MATCH "\"([0-9]+(:?\\.[0-9]+)*)\"" _tmp "${verLine}")
    if (NOT _tmp)
        message(FATAL_ERROR "Could not detect version number from ${_verFile}")
    endif()
    set(${_outVarVersion} ${CMAKE_MATCH_1} PARENT_SCOPE)
endfunction()
