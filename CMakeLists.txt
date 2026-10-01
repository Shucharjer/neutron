cmake_minimum_required(VERSION 3.21)

if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20 CACHE STRING "The C++ standard to use")
endif()

project(
  neutron
  DESCRIPTION
    "A headers-only util library in C++20, including reflection, structures, and some magic."
  LANGUAGES CXX)

# CMake

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

# options

option(NEUTRON_INCLUDE_HEADERS "View header files as source files" OFF)
option(NEUTRON_INSTALL "Install neutron" ON)
option(NEUTRON_BUILD_DOCS "Build documentation for neutron" OFF)
option(NEUTRON_USES_STDEXEC "uses stdexec as execution control library implementation, which supports c++20" OFF)
option(NEUTRON_USES_BEMAN_EXECUTION "uses beman::execution as execution control library implementation, which needs a compiler that supports at least c++23" OFF)
option(NEUTRON_ALIGNED_ALLOCATION "specify whether containers like 'archetype' uses ::operator new(size, align), if set OFF, allocate via allocator" ON)
option(BUILD_NEUTRON_TESTING "Build test for neutron" OFF)
option(BUILD_NEUTRON_BENCHMARK "Build benchmark for neutron" OFF)

# configurate options

include(CheckCXXCompilerFlag)

set(NEUTRON_REFLECTION_FLAGS "")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(NEUTRON_REFLECTION_OPTION "-freflection")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(NEUTRON_REFLECTION_OPTION "-freflection-latest")
endif()

check_cxx_compiler_flag("-std=c++26" NEUTRON_CXX26_SUPPORTED)

if(NEUTRON_CXX26_SUPPORTED AND NEUTRON_REFLECTION_OPTION)
  set(NEUTRON_REFLECTION_CHECK_SOURCE_DIR
      "${CMAKE_CURRENT_BINARY_DIR}/neutron_reflection_check_source")
  file(MAKE_DIRECTORY "${NEUTRON_REFLECTION_CHECK_SOURCE_DIR}")
  file(WRITE "${NEUTRON_REFLECTION_CHECK_SOURCE_DIR}/CMakeLists.txt"
    "cmake_minimum_required(VERSION 3.21)\n"
    "project(neutron_reflection_check LANGUAGES CXX)\n"
    "set(CMAKE_CXX_STANDARD 26)\n"
    "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
    "set(CMAKE_CXX_EXTENSIONS OFF)\n"
    "add_executable(neutron_reflection_check neutron_reflection_check.cpp)\n"
    "target_compile_options(neutron_reflection_check PRIVATE ${NEUTRON_REFLECTION_OPTION} -include meta)\n")
  file(WRITE "${NEUTRON_REFLECTION_CHECK_SOURCE_DIR}/neutron_reflection_check.cpp"
    "#include <meta>\n"
    "#if !((defined(__cpp_lib_reflection) && __cpp_lib_reflection >= 202506L) || (defined(__has_feature) && __has_feature(reflection) && __cplusplus >= 202502L))\n"
    "#error compiler reflection is not enabled\n"
    "#endif\n"
    "int main() { constexpr auto info = ^^int; }\n")
  try_compile(
    NEUTRON_REFLECTION_CHECK
    "${CMAKE_CURRENT_BINARY_DIR}/neutron_reflection_check"
    "${NEUTRON_REFLECTION_CHECK_SOURCE_DIR}"
    neutron_reflection_check
    OUTPUT_VARIABLE NEUTRON_REFLECTION_CHECK_OUTPUT)
  if(NEUTRON_REFLECTION_CHECK)
    set(NEUTRON_REFLECTION_FLAGS
        "${NEUTRON_REFLECTION_OPTION};-include;meta")
  endif()
endif()

if(NEUTRON_REFLECTION_FLAGS)
  set(NEUTRON_REFLECTION_SUPPORTED ON)
else()
  set(NEUTRON_REFLECTION_SUPPORTED OFF)
endif()

# project source

add_library(neutron INTERFACE)
add_library(atom::neutron ALIAS neutron)

target_include_directories(
  neutron INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set_target_properties(
  neutron
  PROPERTIES EXPORT_NAME "neutron"
             LINKER_LANGUAGE CXX)

if(MINGW AND CMAKE_CXX_COMPILER_ID MATCHES GNU)
  target_link_libraries(neutron INTERFACE stdc++exp)
elseif(MSVC)
  target_compile_options(neutron INTERFACE /permissive- /Zc:__cplusplus)
endif()

# options support

if(NEUTRON_INCLUDE_HEADERS)
  file(GLIB_RECURSE NEUTRON_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp)
  foreach(src_file ${NEUTRON_SRC_FILES})
    target_sources(neutron INTERFACE $<BUILD_INTERFACE:${src_file}>)
  endforeach()
endif()

if(NEUTRON_BUILD_DOCS)
  set(DOXY_SOURCE_DIRECTORY ${NEUTRON_SOURCE_DIR}/include)
  set(DOXY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)
  set(DOXY_CSS_DIRECTORY ${doxygen-awesome-css_INCLUDE_DIR})

  configure_file(doxy.in doxy.cfg @ONLY)

  add_custom_target(
    documents
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxy.cfg
    WORKING_DIRECTORY ${NEUTRON_SOURCE_DIR}
    VERBATIM)
endif()

if(NEUTRON_USES_STDEXEC)
  set(STDEXEC_BUILD_EXAMPLES OFF)
  include(FetchContent)
  FetchContent_Declare(
    stdexec
    GIT_REPOSITORY https://github.com/NVIDIA/stdexec.git
    GIT_TAG main
    GIT_SHALLOW 1
  )
  FetchContent_MakeAvailable(stdexec)
  target_link_libraries(neutron INTERFACE STDEXEC::stdexec)
elseif(NEUTRON_USES_BEMAN_EXECUTION)
  include(FetchContent)
  FetchContent_Declare(
    beman_execution
    GIT_REPOSITORY https://github.com/bemanproject/execution.git
    GIT_TAG main
    GIT_SHALLOW 1
  )
  FetchContent_MakeAvailable(beman_execution)
  target_link_libraries(neutron INTERFACE beman::execution_headers)
endif()

target_compile_definitions(neutron INTERFACE
  $<$<BOOL:${NEUTRON_ALIGNED_ALLOCATION}>:NEUTRON_ALIGNED_ALLOCATION>)

include(exec)

if(BUILD_NEUTRON_TESTING)
  enable_testing()
  option(ATOM_TEST_SEE_ALL_TEMPLATE_BACKTRACE OFF)
  set(tests_dir ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  set(tests_output_dir ${CMAKE_CURRENT_BINARY_DIR}/tests)
  exec_standard_dir(${tests_dir} "tests." ${tests_output_dir} true neutron)
  if(ATOM_TEST_SEE_ALL_TEMPLATE_BACKTRACE)
    exec_dir_options(${tests_dir} "tests." -ftemplate-backtrace-limit=0)
  endif()
  add_executable(tests.reflection.name_of-cxx20 ${tests_dir}/ver/reflection/name_of.cpp)
  target_link_libraries(tests.reflection.name_of-cxx20 PRIVATE neutron)
  set_target_properties(
    tests.reflection.name_of-cxx20
    PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  if(NEUTRON_REFLECTION_SUPPORTED)
    add_executable(tests.reflection.name_of-cxx26
                   ${tests_dir}/ver/reflection/name_of.cpp)
    target_link_libraries(tests.reflection.name_of-cxx26 PRIVATE neutron)
    set_target_properties(
      tests.reflection.name_of-cxx26
      PROPERTIES CXX_STANDARD 26 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    target_compile_options(tests.reflection.name_of-cxx26
                           PRIVATE ${NEUTRON_REFLECTION_FLAGS})

    add_custom_target(tests.reflection.name_of.compare
      COMMAND ${CMAKE_COMMAND}
        -DCXX20=$<TARGET_FILE:tests.reflection.name_of-cxx20>
        -DCXX26=$<TARGET_FILE:tests.reflection.name_of-cxx26>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/compare_reflection_name_of.cmake
      DEPENDS tests.reflection.name_of-cxx20
              tests.reflection.name_of-cxx26
      USES_TERMINAL)
  endif()

  option(BUILD_NEUTRON_SIMDJSON_COMPAT OFF)
  if (BUILD_NEUTRON_SIMDJSON_COMPAT)
    FetchContent_Declare(
      simdjson
      GIT_REPOSITORY https://github.com/simdjson/simdjson.git
      GIT_TAG master
      GIT_SHALLOW 1
    )
    FetchContent_MakeAvailable(simdjson)
    target_link_libraries(tests.reflection.serialize.simdjson PRIVATE simdjson)
  endif()
endif()

if(BUILD_NEUTRON_BENCHMARK)
  set(BENCHMARK_ENABLE_TESTING OFF)
  set(BENCHMARK_INSTALL_DOCS OFF)
  set(BENCHMARK_INSTALL_TOOLS OFF)
  set(INSTALL_GTEST OFF)
  include(FetchContent)
  # set(FETCHCONTENT_FULLY_DISCONNECTED ON)
  FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG main
    GIT_SHALLOW 1)
  set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON)
  FetchContent_MakeAvailable(googlebenchmark)
  set(benchmark_dir ${CMAKE_CURRENT_SOURCE_DIR}/benchmark)
  set(benchmark_output_dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark)
  exec_standard_dir(${benchmark_dir} "bm-" ${benchmark_output_dir} false neutron benchmark::benchmark)
endif()
