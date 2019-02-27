cmake_minimum_required(VERSION 3.5)
project(cb LANGUAGES CXX)

# 
# CMAKE_C_COMPILER

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(UV REQUIRED)

list(APPEND libraries ${UV_LIBRARIES})

include_directories(${UV_INCLUDE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/uvw/src)
include_directories(${PROJECT_SOURCE_DIR}) # cb.hpp

aux_source_directory(test source_list)
foreach(source_file ${source_list})
  string(REGEX REPLACE "/" "_" target ${source_file})
  string(REGEX REPLACE ".cpp" "" target ${target})
  add_executable(${target} ${source_file})
  # how do we set this more generally; not per-target
  set_target_properties(${target} PROPERTIES CXX_STANDARD 14)
  set_target_properties(${target} PROPERTIES CXX_STANDARD_REQUIRED ON)
  target_link_libraries(${target} ${libraries})
endforeach(source_file)
