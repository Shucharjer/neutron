project(
  neutron
  DESCRIPTION
    "A headers-only util library in C++20, including reflection, structures, and some magic."
  LANGUAGES CXX)

# CMake

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules)

# options

option(NEUTRON_INCLUDE_HEADERS OFF)
option(NEUTRON_INSTALL ON)
option(NEUTRON_BUILD_DOCS "Build documentation for neutron" OFF)
option(BUILD_NEUTRON_TESTING "Build test for neutron" OFF)
option(BUILD_NEUTRON_BENCHMARK "Build benchmark for neutron" OFF)

# configurate options

include(CMakePushCheckState)
include(CheckCXXSourceCompiles)

# project source

add_library(neutron INTERFACE)
add_library(atom::neutron ALIAS neutron)

target_include_directories(
  neutron INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

find_package(stdexec CONFIG REQUIRED)
target_link_libraries(neutron INTERFACE STDEXEC::stdexec)

set_target_properties(
  neutron
  PROPERTIES EXPORT_NAME "neutron"
             LINKER_LANGUAGE CXX)

# options support

if(NEUTRON_INCLUDE_HEADERS)
  set(inc_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_sources(
    Utils
    INTERFACE
      # the fellow line is a template
      # $<BUILD_INTERFACE:${inc_dir}>
      $<BUILD_INTERFACE:${inc_dir}/auxiliary.hpp>
      $<BUILD_INTERFACE:${inc_dir}/auxiliary/singleton.hpp>
      $<BUILD_INTERFACE:${inc_dir}/concepts/ranges.hpp>
      $<BUILD_INTERFACE:${inc_dir}/concepts/type.hpp>
      $<BUILD_INTERFACE:${inc_dir}/core.hpp>
      $<BUILD_INTERFACE:${inc_dir}/core/type_traits.hpp>
      $<BUILD_INTERFACE:${inc_dir}/memory.hpp>
      $<BUILD_INTERFACE:${inc_dir}/memory/storage.hpp>
      $<BUILD_INTERFACE:${inc_dir}/misc/timer.hpp>
      $<BUILD_INTERFACE:${inc_dir}/ranges.hpp>
      $<BUILD_INTERFACE:${inc_dir}/ranges/iterator.hpp>
      $<BUILD_INTERFACE:${inc_dir}/ranges/to.hpp>
      $<BUILD_INTERFACE:${inc_dir}/reflection.hpp>
      $<BUILD_INTERFACE:${inc_dir}/signal.hpp>
      $<BUILD_INTERFACE:${inc_dir}/signal/sink.hpp>
      $<BUILD_INTERFACE:${inc_dir}/signal/dispatcher.hpp>
      $<BUILD_INTERFACE:${inc_dir}/structures.hpp>
      $<BUILD_INTERFACE:${inc_dir}/structures/dense_map.hpp>
      $<BUILD_INTERFACE:${inc_dir}/structures/dense_set.hpp>
      $<BUILD_INTERFACE:${inc_dir}/structures/tstring.hpp>
      $<BUILD_INTERFACE:${inc_dir}/thread.hpp>
      $<BUILD_INTERFACE:${inc_dir}/thread/coroutine.hpp>
      $<BUILD_INTERFACE:${inc_dir}/thread/lock_keeper.hpp>
      $<BUILD_INTERFACE:${inc_dir}/thread/lock.hpp>
      $<BUILD_INTERFACE:${inc_dir}/thread/thread_pool.hpp>)
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


include(exec)

if(BUILD_NEUTRON_TESTING)
  enable_testing()
  option(ATOM_TEST_SEE_ALL_TEMPLATE_BACKTRACE OFF)
  set(test_dir ${CMAKE_CURRENT_SOURCE_DIR}/test)
  set(test_output_dir ${CMAKE_CURRENT_BINARY_DIR}/test)
  exec_dir(${test_dir} "test-" ${test_output_dir} true neutron)
  if(ATOM_TEST_SEE_ALL_TEMPLATE_BACKTRACE)
    exec_dir_options(${test_dir} "test-" -ftemplate-backtrace-limit=0)
  endif()
endif()

if(BUILD_NEUTRON_BENCHMARK)
  set(benchmark_dir ${CMAKE_CURRENT_SOURCE_DIR}/benchmark)
  set(benchmark_output_dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark)
  exec_dir(${benchmark_dir} "bm-" ${benchmark_output_dir} false neutron benchmark::benchmark)
endif()
