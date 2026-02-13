cmake_minimum_required(VERSION 3.24)

include(tools/preConfig.cmake)
include(tools/checkSDK.cmake)
include(tools/linkEngine.cmake)

add_subdirectory(engine)
add_subdirectory(editor)