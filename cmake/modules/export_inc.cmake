# Export internal implementation to the specified position
#
# export_inc(<internal_directory> <output_position> <suffix> [PUBLIC]
#            [EXCLUDE <file>...])
#
# A PUBLIC header only provides the macros and imports the `neutron` module
# when the module is used.
function(export_inc internal_directory output_position suffix)
    cmake_parse_arguments(ARG "PUBLIC" "" "EXCLUDE" ${ARGN})
    file(GLOB hdr_files CONFIGURE_DEPENDS "${internal_directory}/*.${suffix}")
    list(REMOVE_ITEM hdr_files ${output_position} ${ARG_EXCLUDE})
    list(SORT hdr_files)

    set(include_root "${PROJECT_SOURCE_DIR}/include")
    set(content "#pragma once\n// IWYU pragma: begin_exports\n\n")
    set(indent "")
    if(ARG_PUBLIC)
        string(APPEND content
            "#if defined(ATOM_USES_MODULES) && !defined(ATOM_BUILDING_MODULE)\n"
            "    #include \"neutron/macros.hpp\"\n"
            "#else\n")
        set(indent "    ")
    endif()

    foreach(hdr_file IN LISTS hdr_files)
        file(RELATIVE_PATH rel_path "${include_root}" "${hdr_file}")
        string(APPEND content "${indent}#include \"${rel_path}\"\n")
    endforeach()

    if(ARG_PUBLIC)
        string(APPEND content "#endif\n")
    endif()
    string(APPEND content "\n// IWYU pragma: end_exports\n")

    # Only rewrite the header when its content changes, otherwise every
    # configure would bump its mtime and force dependent targets to rebuild.
    set(tmp_file "${output_position}.tmp")
    file(WRITE "${tmp_file}" "${content}")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${tmp_file}" "${output_position}")
    file(REMOVE "${tmp_file}")
endfunction()
