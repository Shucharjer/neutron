execute_process(
  COMMAND "${CXX20}"
  RESULT_VARIABLE cxx20_result
  OUTPUT_VARIABLE cxx20_output
  ERROR_VARIABLE cxx20_error
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
  COMMAND "${CXX26}"
  RESULT_VARIABLE cxx26_result
  OUTPUT_VARIABLE cxx26_output
  ERROR_VARIABLE cxx26_error
  OUTPUT_STRIP_TRAILING_WHITESPACE)

message(STATUS "--- tests.reflection.name_of: C++20 ---")
message(STATUS "${cxx20_output}")
if(NOT cxx20_result EQUAL 0)
  message(FATAL_ERROR "C++20 executable failed: ${cxx20_error}")
endif()

message(STATUS "--- tests.reflection.name_of: C++26 reflection ---")
message(STATUS "${cxx26_output}")
if(NOT cxx26_result EQUAL 0)
  message(FATAL_ERROR "C++26 reflection executable failed: ${cxx26_error}")
endif()

if("${cxx20_output}" STREQUAL "${cxx26_output}")
  message(STATUS "name_of comparison: MATCH")
else()
  message(FATAL_ERROR "name_of comparison: DIFFERENT OUTPUT")
endif()
