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