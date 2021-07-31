# function for parsing the version from a header file
function(read_version _verFile _outVarVersion)
    file(STRINGS ${_verFile} verLine REGEX "VERSION[ \t]+\"")
    string(REGEX MATCH "\"([0-9]+(:?\\.[0-9]+)*)\"" _tmp "${verLine}")
    if (NOT _tmp)
        message(FATAL_ERROR "Could not detect version number from ${_verFile}")
    endif ()
    set(${_outVarVersion} ${CMAKE_MATCH_1} PARENT_SCOPE)
endfunction()
