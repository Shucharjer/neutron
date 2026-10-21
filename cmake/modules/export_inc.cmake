# Export internal implementation to the specified position
function(export_inc internal_directory output_position suffix)
    file(GLOB hdr_files CONFIGURE_DEPENDS "${internal_directory}/*.${suffix}")
    list(SORT hdr_files)

    set(include_root "${PROJECT_SOURCE_DIR}/include")
    set(content "#pragma once\n// IWYU pragma: begin_exports\n\n")

    foreach(hdr_file IN LISTS hdr_files)
        file(RELATIVE_PATH rel_path "${include_root}" "${hdr_file}")
        string(APPEND content "#include \"${rel_path}\"\n")
    endforeach()

    string(APPEND content "\n// IWYU pragma: end_exports\n")

    # Only rewrite the header when its content changes, otherwise every
    # configure would bump its mtime and force dependent targets to rebuild.
    set(tmp_file "${output_position}.tmp")
    file(WRITE "${tmp_file}" "${content}")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${tmp_file}" "${output_position}")
    file(REMOVE "${tmp_file}")
endfunction()
