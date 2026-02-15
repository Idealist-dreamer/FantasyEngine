cmake_minimum_required(VERSION 3.24)

project(TestMemory VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CodePath ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE SOURCE_FILE
    "${CodePath}/*.h"
    "${CodePath}/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCE_FILE} ${RESOURCE_FILE})

find_package(GTest CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest)

fe_set_output_directory(${PROJECT_NAME})

target_compile_definitions(Engine PUBLIC $<$<CONFIG:Debug>:FE_DEBUG>)

LinkEngine(${PROJECT_NAME})