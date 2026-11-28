function(exec_standard_dir dir prefix output_dir as_test)
  set(libs ${ARGN})

  file(GLOB_RECURSE source RELATIVE ${dir}/src ${dir}/src/*.cpp)
  foreach(src_file ${source})
    string(REPLACE ".cpp" "" exec_base ${src_file})
    string(REPLACE "/" "." exec_name ${exec_base})
    set(exec_name ${prefix}${exec_name})
    add_executable(${exec_name} ${dir}/src/${src_file})
    target_include_directories(${exec_name} PRIVATE $<BUILD_INTERFACE:${dir}/include>)
    target_link_libraries(${exec_name} PRIVATE ${libs})
    set_target_properties(${exec_name} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${output_dir}
    )
    if(as_test)
      add_test(NAME ${exec_name} COMMAND $<TARGET_FILE:${exec_name}>)
      set_tests_properties(${exec_name} PROPERTIES TIMEOUT 30)
    endif()
  endforeach()
endfunction()

function(exec_dir dir prefix output_dir as_test strip_output)
  set(libs ${ARGN})

  file(GLOB_RECURSE source RELATIVE ${dir} ${dir}/*.cpp)
  foreach(src_file ${source})
    string(REPLACE ".cpp" "" exec_base ${src_file})
    string(REPLACE "/" "." exec_name ${exec_base})
    set(exec_name ${prefix}${exec_name})
    add_executable(${exec_name} ${dir}/${src_file})
    target_link_libraries(${exec_name} PRIVATE ${libs})
    set_target_properties(${exec_name} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${output_dir}
    )
    if(as_test)
      add_test(NAME ${exec_name} COMMAND $<TARGET_FILE:${exec_name}>)
      set_tests_properties(${exec_name} PROPERTIES TIMEOUT 30)
    endif()
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND ${strip_output})
      add_custom_command(TARGET ${exec_name}
        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${exec_name}>
        COMMENT "Shrink executable size by stripping debug symbols"
      )
    endif()
  endforeach()
endfunction()

function(exec_standard_dir_options dir prefix)
  set(options ${ARGN})
  file(GLOB_RECURSE source RELATIVE ${dir}/src ${dir}/src/*.cpp)
  foreach(src ${source})
    get_filename_component(exec_raw ${src} NAME_WE)
    string(REPLACE ".cpp" "" exec_base ${exec_raw})
    string(REPLACE "/" "_" exec_name ${exec_base})
    set(exec_name ${prefix}${exec_name})
    target_compile_options(${exec_name} PRIVATE ${options})
  endforeach()
endfunction()

# Like exec_standard_dir, for sources that use the `neutron` module, which LIBS
# provide. The sources listed in UNIT_SOURCES (relative to `${dir}/src`) use
# what the module does not export: each one is compiled as an implementation
# unit of `neutron`, which imports std itself, since a module unit cannot get
# imports from an included file; the `main` of the source has to be
# `extern "C++"`.
function(exec_standard_dir_module dir prefix output_dir as_test)
  cmake_parse_arguments(ARG "" "" "LIBS;UNIT_SOURCES" ${ARGN})

  file(GLOB_RECURSE source RELATIVE ${dir}/src ${dir}/src/*.cpp)
  foreach(src_file ${source})
    string(REPLACE ".cpp" "" exec_base ${src_file})
    string(REPLACE "/" "." exec_name ${exec_base})
    set(exec_name ${prefix}${exec_name})
    if(src_file IN_LIST ARG_UNIT_SOURCES)
      # A source cannot become a module unit conditionally, so it is included
      # by a generated one.
      set(unit_file ${CMAKE_CURRENT_BINARY_DIR}/module_units/${src_file})
      string(CONCAT unit_content
        "// Generated: compiles the source below as an implementation unit.\n"
        "module;\n"
        "// Macros, which modules do not export.\n"
        "#include <cstddef> // offsetof\n"
        "#include \"neutron/detail/utility/macros.hpp\"\n"
        "#if ATOM_HAS_REFLECTION\n"
        "    #include <meta>\n"
        "#else\n"
        "    #include \"neutron/detail/reflection/legacy/user_macros.hpp\"\n"
        "#endif\n"
        "module neutron;\n"
        "import std;\n"
        "#include \"${dir}/src/${src_file}\"\n")
      file(GENERATE OUTPUT ${unit_file} CONTENT "${unit_content}")
      add_executable(${exec_name} ${unit_file})
      # The internal entities of a test may appear in module templates, which
      # is fine since its purview is not shared with other translation units.
      target_compile_options(${exec_name} PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-Wno-subobject-linkage>)
    else()
      add_executable(${exec_name} ${dir}/src/${src_file})
    endif()
    target_include_directories(${exec_name} PRIVATE $<BUILD_INTERFACE:${dir}/include>)
    target_link_libraries(${exec_name} PRIVATE ${ARG_LIBS})
    set_target_properties(${exec_name} PROPERTIES
      CXX_SCAN_FOR_MODULES ON
      RUNTIME_OUTPUT_DIRECTORY ${output_dir}
    )
    if(as_test)
      add_test(NAME ${exec_name} COMMAND $<TARGET_FILE:${exec_name}>)
      set_tests_properties(${exec_name} PROPERTIES TIMEOUT 30)
    endif()
  endforeach()
endfunction()
