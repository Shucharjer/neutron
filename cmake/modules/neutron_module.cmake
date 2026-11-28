include_guard(GLOBAL)

# Detects the standard library the compiler builds against, since the module
# metadata (`*.modules.json`) is shipped per standard library.
function(_neutron_detect_stdlib out_var)
  if(CMAKE_CXX_STANDARD_LIBRARY)
    set(${out_var} "${CMAKE_CXX_STANDARD_LIBRARY}" PARENT_SCOPE)
    return()
  endif()
  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles(
    "#include <version>\n#ifndef _LIBCPP_VERSION\n#error\n#endif\nint main() {}"
    NEUTRON_STDLIB_IS_LIBCXX)
  if(NEUTRON_STDLIB_IS_LIBCXX)
    set(${out_var} "libc++" PARENT_SCOPE)
  else()
    set(${out_var} "libstdc++" PARENT_SCOPE)
  endif()
endfunction()

# neutron_add_std_module(<target>)
#
# Builds the `std` and `std.compat` modules from the sources the standard
# library ships, for the C++ standard of the build. Unlike CMake's own
# `import std` support, this also works in C++20, where both libc++ and
# libstdc++ provide the modules as an extension.
function(neutron_add_std_module target)
  _neutron_detect_stdlib(stdlib)
  if(NOT stdlib MATCHES "^(libc\\+\\+|libstdc\\+\\+)$")
    message(FATAL_ERROR
      "NEUTRON_BUILD_MODULE needs libc++ or libstdc++, but the standard "
      "library is '${stdlib}'.")
  endif()

  separate_arguments(cxx_flags NATIVE_COMMAND "${CMAKE_CXX_FLAGS}")
  execute_process(
    COMMAND "${CMAKE_CXX_COMPILER}" ${cxx_flags}
            "-print-file-name=${stdlib}.modules.json"
    OUTPUT_VARIABLE modules_json
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE result)
  if(result OR NOT IS_ABSOLUTE "${modules_json}" OR NOT EXISTS "${modules_json}")
    message(FATAL_ERROR
      "NEUTRON_BUILD_MODULE needs the module sources of ${stdlib}, but "
      "'${stdlib}.modules.json' is not found by '${CMAKE_CXX_COMPILER}'.")
  endif()
  cmake_path(GET modules_json PARENT_PATH json_dir)
  file(READ "${modules_json}" json)

  add_library(${target} STATIC)
  set(base_dirs "")
  set(sources "")
  string(JSON count LENGTH "${json}" modules)
  math(EXPR last "${count} - 1")
  foreach(index RANGE ${last})
    string(JSON source GET "${json}" modules ${index} source-path)
    cmake_path(ABSOLUTE_PATH source BASE_DIRECTORY "${json_dir}" NORMALIZE)
    cmake_path(GET source PARENT_PATH source_dir)
    list(APPEND base_dirs "${source_dir}")
    list(APPEND sources "${source}")

    string(JSON includes ERROR_VARIABLE no_includes GET "${json}" modules
           ${index} local-arguments system-include-directories)
    if(NOT no_includes)
      string(JSON include_count LENGTH "${includes}")
      math(EXPR include_last "${include_count} - 1")
      foreach(include_index RANGE ${include_last})
        string(JSON include GET "${includes}" ${include_index})
        cmake_path(ABSOLUTE_PATH include BASE_DIRECTORY "${json_dir}"
                   NORMALIZE)
        target_include_directories(${target} SYSTEM PRIVATE "${include}")
      endforeach()
    endif()
  endforeach()
  list(REMOVE_DUPLICATES base_dirs)

  target_sources(${target} PUBLIC FILE_SET CXX_MODULES BASE_DIRS ${base_dirs}
                                  FILES ${sources})
  target_compile_features(${target} PUBLIC cxx_std_20)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # The standard library may declare reserved module names.
    target_compile_options(${target} PRIVATE -Wno-reserved-module-identifier)
  endif()
  set_target_properties(${target} PROPERTIES CXX_SCAN_FOR_MODULES ON)
endfunction()
