cmake_minimum_required(VERSION 3.5)
project(cb LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

#add_library(${PROJECT_NAME} SHARED ${library_source})
#target_link_libraries(${PROJECT_NAME} ${library_library})

find_package(UV REQUIRED)
list(APPEND libraries ${UV_LIBRARIES})
include_directories(${UV_INCLUDE_DIR})
include_directories(uvw/src)
include_directories(.)

aux_source_directory(test source_list)
foreach(source_file ${source_list})
  string(REGEX REPLACE "/" "_" target ${source_file})
  string(REGEX REPLACE ".cpp" "" target ${target})
  add_executable(${target} ${source_file})
  set_target_properties(${target} PROPERTIES CXX_STANDARD 14)
  set_target_properties(${target} PROPERTIES CXX_STANDARD_REQUIRED ON)
  target_link_libraries(${target} ${libraries})
endforeach(source_file)
