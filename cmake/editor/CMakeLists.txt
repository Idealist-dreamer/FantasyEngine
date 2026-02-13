cmake_minimum_required(VERSION 3.24)

project(Editor VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CodePath ${CMAKE_SOURCE_DIR}/editor)

file(GLOB_RECURSE SOURCE_FILE
    "${CodePath}/*.h"
    "${CodePath}/*.cpp"
    "${CodePath}/*.ui")
file(GLOB_RECURSE RESOURCE_FILE
    "${CMAKE_SOURCE_DIR}/Resource/Editor/*.rc")

add_executable(Editor ${SOURCE_FILE} ${RESOURCE_FILE})

fe_set_output_directory(Editor)

target_link_libraries(Editor PRIVATE 
    Engine
)