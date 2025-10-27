project(
  neutron
  DESCRIPTION
    "A headers-only util library in C++20, including reflection, structures, and some magic."
  LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# CMake

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules)

# common options

option(NEUTRON_INCLUDE_HEADERS OFF)
option(NEUTRON_INSTALL ON)
option(NEUTRON_BUILD_DOCS "Build documentation for neutron" OFF)
option(NEUTRON_BUILD_TEST "Build test for neutron" OFF)
option(NEUTRON_BUILD_BENCHMARK "Build benchmark for neutron" OFF)

# configurate options

include(CMakePushCheckState)
include(CheckCXXSourceCompiles)

add_library(neutron INTERFACE)
add_library(atom::neutron ALIAS neutron)

target_include_directories(
  neutron INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set_target_properties(
  neutron
  PROPERTIES EXPORT_NAME "neutron"
             LINKER_LANGUAGE CXX)

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

if(NEUTRON_INSTALL)

  include(JoinPaths)

  set(Utils_PKGCONFIG ${CMAKE_CURRENT_BINARY_DIR}/utils.pc)

  join_paths(Utils_PKGCONFIG_INCLUDEDIR "\${prefix}" "CMAKE_INSTALL_INCLUDEDIR")

  configure_file(${Utils_SOURCE_DIR}/cmake/in/utils.pc.in ${Utils_PKGCONFIG}
                 @ONLY)

  install(FILES ${Utils_PKGCONFIG}
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

  include(CMakePackageConfigHelpers)

  install(
    TARGETS Utils
    EXPORT UtilsTargets
    ARCHIVE DESTINATION ${INSTALL_LIBDIR})

  configure_package_config_file(
    ${Utils_SOURCE_DIR}/cmake/in/UtilsConfig.cmake.in UtilsConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/Utils/cmake)

  export(
    EXPORT UtilsTargets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/UtilsTargets.cmake
    NAMESPACE atom::)

  install(
    EXPORT UtilsTargets
    FILE UtilsTargets.cmake
    NAMESPACE atom::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/Utils/cmake)

  install(FILES ${PROJECT_BINARY_DIR}/UtilsConfig.cmake
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/Utils/cmake)

  install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/atom/utils
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp")

  export(PACKAGE Utils)

endif()

if(NEUTRON_BUILD_DOCS)
  find_package(Doxygen REQUIRED dot)
  if(NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Required to build documents but doxygen is not found!")
  endif()
  include(FetchContent)

  FetchContent_Declare(
    doxygen-awesome-css
    GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css
    GIT_TAG main
    GIT_SHALLOW 1)
  FetchContent_MakeAvailable(doxygen-awesome-css)

  FetchContent_GetProperties(doxygen-awesome-css)

  if(NOT doxygen-awesome-css_POPULATED)
    FetchContent_Populate(doxygen-awesome-css)
    set(doxygen-awesome-css_INCLUDE_DIR ${doxygen-awesome-css_SOURCE_DIR})
  endif()

  set(DOXY_SOURCE_DIRECTORY ${Utils_SOURCE_DIR}/include)
  set(DOXY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)
  set(DOXY_CSS_DIRECTORY ${doxygen-awesome-css_INCLUDE_DIR})

  configure_file(doxy.in doxy.cfg @ONLY)

  add_custom_target(
    documents
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxy.cfg
    WORKING_DIRECTORY ${Utils_SOURCE_DIR}
    VERBATIM)
endif()

include(CTest)
enable_testing()

include(exec)

if(NEUTRON_BUILD_TEST)
  option(ATOM_TEST_SEE_ALL_TEMPLATE_BACKTRACE OFF)
  set(test_dir ${CMAKE_CURRENT_SOURCE_DIR}/test)
  set(test_output_dir ${CMAKE_CURRENT_BINARY_DIR}/test)
  exec_dir(${test_dir} "test-" ${test_output_dir} true neutron)
  if(ATOM_TEST_SEE_ALL_TEMPLATE_BACKTRACE)
    exec_dir_options(${test_dir} "test-" ftemplate-backtrace-limit=0)
  endif()
endif()

if(NEUTRON_BUILD_BENCHMARK)
  set(benchmark_dir ${CMAKE_CURRENT_SOURCE_DIR}/benchmark)
  set(benchmark_output_dir ${CMAKE_CURRENT_BINARY_DIR}/benchmark)
  exec_dir(${benchmark_dir} "benchmark-" ${benchmark_output_dir} false neutron benchmark::benchmark)
endif()
