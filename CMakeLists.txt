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
option(NEUTRON_BUILD_MODULE "Build neutron, std and the execution backend as C++20 named modules; the tests then import them" OFF)
option(BUILD_NEUTRON_TESTING "Build test for neutron" OFF)
option(BUILD_NEUTRON_BENCHMARK "Build benchmark for neutron" OFF)
option(BUILD_NEUTRON_EXAMPLES "Build examples for neutron" OFF)

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
  # The reflection header is <meta> in the standard, but p2996 Clang
  # toolchains may expose <experimental/meta> instead. Pre-include a shim
  # that picks whichever is available so one flag works for both layouts.
  set(NEUTRON_REFLECTION_PREINCLUDE
      "${CMAKE_CURRENT_BINARY_DIR}/neutron_reflection_preinclude.hpp")
  file(WRITE "${NEUTRON_REFLECTION_PREINCLUDE}"
    "#if __has_include(<meta>)\n"
    "#include <meta>\n"
    "#elif __has_include(<experimental/meta>)\n"
    "#include <experimental/meta>\n"
    "#endif\n")
  file(WRITE "${NEUTRON_REFLECTION_CHECK_SOURCE_DIR}/CMakeLists.txt"
    "cmake_minimum_required(VERSION 3.21)\n"
    "project(neutron_reflection_check LANGUAGES CXX)\n"
    "set(CMAKE_CXX_STANDARD 26)\n"
    "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
    "set(CMAKE_CXX_EXTENSIONS OFF)\n"
    "add_executable(neutron_reflection_check neutron_reflection_check.cpp)\n"
    "target_compile_options(neutron_reflection_check PRIVATE ${NEUTRON_REFLECTION_OPTION} -include \"${NEUTRON_REFLECTION_PREINCLUDE}\")\n")
  file(WRITE "${NEUTRON_REFLECTION_CHECK_SOURCE_DIR}/neutron_reflection_check.cpp"
    "#if !(__has_include(<meta>) || __has_include(<experimental/meta>))\n"
    "#error no reflection header found\n"
    "#endif\n"
    "#if !((defined(__cpp_lib_reflection) && __cpp_lib_reflection >= 202506L) || (defined(__has_feature) && __has_feature(reflection)))\n"
    "#error compiler reflection is not enabled\n"
    "#endif\n"
    "int main() { constexpr auto info = ^^int; }\n")
  try_compile(
    NEUTRON_REFLECTION_CHECK
    "${CMAKE_CURRENT_BINARY_DIR}/neutron_reflection_check"
    "${NEUTRON_REFLECTION_CHECK_SOURCE_DIR}"
    neutron_reflection_check
    CMAKE_FLAGS "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
    OUTPUT_VARIABLE NEUTRON_REFLECTION_CHECK_OUTPUT)
  if(NEUTRON_REFLECTION_CHECK)
    set(NEUTRON_REFLECTION_FLAGS
        "${NEUTRON_REFLECTION_OPTION};-include;${NEUTRON_REFLECTION_PREINCLUDE}")
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
  # /Zc:preprocessor: the reflection macros need the conforming preprocessor
  target_compile_options(neutron INTERFACE /permissive- /Zc:__cplusplus /Zc:preprocessor)
endif()

include(export_inc)
set(NEUTRON_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/neutron)
function(export_as_inc folder)
  export_inc(${NEUTRON_INC_DIR}/detail/${folder} ${NEUTRON_INC_DIR}/${folder}.hpp hpp PUBLIC)
endfunction()
export_as_inc(algorithm)
export_as_inc(concepts)
export_inc(${NEUTRON_INC_DIR}/detail/ecs/compile-time ${NEUTRON_INC_DIR}/detail/ecs/compile-time.hpp hpp)
export_inc(${NEUTRON_INC_DIR}/detail/ecs/concepts ${NEUTRON_INC_DIR}/detail/ecs/concepts.hpp hpp)
export_inc(${NEUTRON_INC_DIR}/detail/ecs/core ${NEUTRON_INC_DIR}/detail/ecs/core.hpp hpp)
export_inc(${NEUTRON_INC_DIR}/detail/ecs/params ${NEUTRON_INC_DIR}/detail/ecs/params.hpp hpp)
export_inc(${NEUTRON_INC_DIR}/detail/ecs/runtime ${NEUTRON_INC_DIR}/detail/ecs/runtime.hpp hpp)
export_inc(${NEUTRON_INC_DIR}/detail/ecs/utility ${NEUTRON_INC_DIR}/detail/ecs/utility.hpp hpp)
export_as_inc(ecs)
export_as_inc(execution)
export_as_inc(execution_resource)
export_as_inc(fstream)
export_as_inc(functional)
export_as_inc(interprocess)
export_as_inc(iterator)
export_as_inc(lockfree)
export_as_inc(memory)
export_as_inc(metafn)
export_as_inc(parallel)
export_as_inc(polymorphic)
export_inc(${NEUTRON_INC_DIR}/detail/ranges/views ${NEUTRON_INC_DIR}/detail/ranges/views.hpp hpp)
export_as_inc(ranges)
export_as_inc(string)
export_as_inc(tuple)
export_as_inc(type_traits)
export_as_inc(utility)

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

if(NEUTRON_BUILD_MODULE)
  if(NOT CMAKE_GENERATOR MATCHES "Ninja")
    message(FATAL_ERROR
      "NEUTRON_BUILD_MODULE requires a Ninja generator, but the generator is "
      "'${CMAKE_GENERATOR}'.")
  endif()
  if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
      CMAKE_CXX_COMPILER_VERSION VERSION_LESS 15) OR
     (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
      CMAKE_CXX_COMPILER_VERSION VERSION_LESS 18.1.2))
    message(FATAL_ERROR
      "NEUTRON_BUILD_MODULE requires GCC 15+ or Clang 18.1.2+, which ship the "
      "`std` module, but the compiler is "
      "'${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}'.")
  endif()
  include(neutron_module)
  neutron_add_std_module(neutron_std_module)
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
  if(NEUTRON_BUILD_MODULE)
    # stdexec's own module option requires C++23, so its module interface is
    # built here on top of neutron's `std` module instead.
    add_library(neutron_stdexec_module STATIC)
    target_sources(
      neutron_stdexec_module
      PUBLIC FILE_SET CXX_MODULES
             BASE_DIRS ${stdexec_SOURCE_DIR}/modules
             FILES ${stdexec_SOURCE_DIR}/modules/stdexec.cppm)
    # Also turns the stdexec headers of the consumers into `import stdexec;`.
    target_compile_definitions(neutron_stdexec_module
                               PUBLIC STDEXEC_BUILD_MODULES)
    target_link_libraries(neutron_stdexec_module PUBLIC STDEXEC::stdexec
                                                        neutron_std_module)
    set_target_properties(neutron_stdexec_module
                          PROPERTIES CXX_SCAN_FOR_MODULES ON)
    set(NEUTRON_EXECUTION_MODULE neutron_stdexec_module)
  endif()
elseif(NEUTRON_USES_BEMAN_EXECUTION)
  if(CMAKE_CXX_STANDARD LESS 23)
    message(FATAL_ERROR
      "NEUTRON_USES_BEMAN_EXECUTION requires C++23 or later, but "
      "CMAKE_CXX_STANDARD is '${CMAKE_CXX_STANDARD}'. ")
  endif()
  include(FetchContent)
  if(NEUTRON_BUILD_MODULE)
    # Build beman's modules with `import std;`, but skip its detection of
    # CMake's `import std` support: neutron provides the `std` module itself.
    set(BEMAN_USE_MODULES ON)
    set(BEMAN_USE_STD_MODULE OFF)
    set(BEMAN_HAS_IMPORT_STD ON)
  else()
    set(BEMAN_USE_MODULES OFF)
  endif()
  FetchContent_Declare(
    beman_execution
    GIT_REPOSITORY https://github.com/bemanproject/execution.git
    GIT_TAG main
    GIT_SHALLOW 1
  )
  FetchContent_MakeAvailable(beman_execution)
  target_link_libraries(neutron INTERFACE beman::execution_headers)
  if(NEUTRON_BUILD_MODULE)
    # beman hides its symbols, unlike the headers that neutron otherwise uses,
    # which makes GCC warn about neutron types holding beman ones.
    set_target_properties(
      beman.execution
      PROPERTIES CXX_MODULE_STD OFF
                 CXX_VISIBILITY_PRESET default
                 VISIBILITY_INLINES_HIDDEN OFF)
    # Build tree only, beman installs its targets.
    target_link_libraries(beman.execution
                          PUBLIC $<BUILD_INTERFACE:neutron_std_module>)
    set(NEUTRON_EXECUTION_MODULE beman.execution)
  endif()
endif()

if(NEUTRON_BUILD_MODULE)
  # Every public header, included by the module interface unit.
  set(NEUTRON_MODULE_INC_DIR ${CMAKE_CURRENT_BINARY_DIR}/neutron_module)
  export_inc(${NEUTRON_INC_DIR} ${NEUTRON_MODULE_INC_DIR}/neutron_all.hpp hpp
             EXCLUDE ${NEUTRON_INC_DIR}/macros.hpp)

  add_library(neutron_module STATIC)
  add_library(atom::neutron_module ALIAS neutron_module)
  target_include_directories(neutron_module PRIVATE ${NEUTRON_MODULE_INC_DIR})
  target_sources(
    neutron_module
    PUBLIC FILE_SET CXX_MODULES
           BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/modules
           FILES ${CMAKE_CURRENT_SOURCE_DIR}/modules/neutron.cppm)
  target_link_libraries(neutron_module PUBLIC neutron neutron_std_module
                                              ${NEUTRON_EXECUTION_MODULE})
  # The headers of neutron become `import neutron;` in the consumers.
  target_compile_definitions(neutron_module PUBLIC ATOM_USES_MODULES=1)
  set_target_properties(
    neutron_module
    PROPERTIES EXPORT_NAME "neutron_module"
               CXX_SCAN_FOR_MODULES ON)
endif()

include(exec)

if(BUILD_NEUTRON_TESTING)
  enable_testing()
  option(ATOM_TEST_SEE_ALL_TEMPLATE_BACKTRACE OFF)
  set(tests_dir ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  set(tests_output_dir ${CMAKE_CURRENT_BINARY_DIR}/tests)
  if(NEUTRON_BUILD_MODULE)
    # The tests import neutron and std. These ones check internals, which the
    # module does not export, so they are implementation units of neutron.
    set(internal_tests
        constexpr.cpp
        ecs/compile-time/desc_validator.cpp
        ecs/compile-time/query_cache.cpp
        ecs/core/slice.cpp
        ecs/core/world_base.cpp
        ecs/params/construct/local.cpp
        ecs/runtime/run_env.cpp
        ecs/runtime/runtime.cpp
        ecs/utility/anchor.cpp
        fstream/file_map.cpp
        fstream/file_window.cpp
        fstream/fmbuf.cpp
        fstream/fmstream.cpp
        fstream/ifmstream.cpp
        fstream/ofmstream.cpp
        iterator/iter_wrapper.cpp
        lockfree/inplace_mpsc_queue.cpp
        lockfree/inplace_spsc_queue.cpp
        lockfree/intrusive_mpsc_queue.cpp
        lockfree/node.cpp
        mask.cpp
        memory/freeable_bytes.cpp
        memory/object_pool.cpp
        ranges/adaptor_closure.cpp
        ranges/concepts.cpp
        reflection/hash.cpp
        reflection/hash_fn.cpp
        reflection/hash_t.cpp
        reflection/offset_helper.cpp
        utility/compressed_element.cpp
        utility/get.cpp
        utility/pipeline.cpp)
    exec_standard_dir_module(${tests_dir} "tests." ${tests_output_dir} true
                             LIBS neutron_module
                             UNIT_SOURCES ${internal_tests})
  else()
    exec_standard_dir(${tests_dir} "tests." ${tests_output_dir} true neutron)
  endif()
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

if(BUILD_NEUTRON_EXAMPLES)
  # Examples are registered as tests too, so ctest checks that they still run.
  exec_dir(${CMAKE_CURRENT_SOURCE_DIR}/examples "examples."
           ${CMAKE_CURRENT_BINARY_DIR}/examples true false neutron)
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
