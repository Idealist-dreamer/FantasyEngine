cmake_minimum_required(VERSION 3.24)

project(Engine VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CodePath ${CMAKE_SOURCE_DIR}/engine)

file(GLOB_RECURSE HEAD_FILE
    "${CodePath}/*.h")
file(GLOB_RECURSE SOURCE_FILE
    "${CodePath}/*.cpp")

add_library(Engine STATIC ${SOURCE_FILE})

# Target config
fe_set_output_directory(Engine)
set_target_properties(Engine PROPERTIES DEBUG_POSTFIX "d")

target_compile_definitions(Engine PUBLIC UNICODE _UNICODE)
target_compile_definitions(Engine PUBLIC $<$<CONFIG:Debug>:FE_DEBUG>)

if(MSVC)
    target_compile_options(Engine PRIVATE /permissive)
endif()

option(FE_OP_USE_EASTL "use eastl" ON)
if(FE_OP_USE_EASTL)
    target_compile_definitions(Engine PUBLIC FE_USE_EASTL)
    target_compile_definitions(Engine PUBLIC EASTL_EASTDC_VSNPRINTF=0)
endif()

option(FE_OP_OVER_NEW_DEL_MIMALLOC "use mimalloc overide new and delte" ON)
if(FE_OP_OVER_NEW_DEL_MIMALLOC)
    target_compile_definitions(Engine PUBLIC FE_OVER_NEW_DEL_MIMALLOC)
endif()

target_include_directories(Engine
    PUBLIC
    ${CMAKE_SOURCE_DIR}
)

include(LinkSDK.cmake)  
fe_link_engine_sdks(Engine)

include(FlatGener.cmake)

# find fbs file
file(GLOB_RECURSE FBS_FILES
    "${CodePath}/*.fbs"
)

FlatGenerateFun(
    TARGET EngineGeneratedHeaders
    SCHEMAS ${FBS_FILES}
    SCHEMA_ROOT "${CodePath}"
    OUTPUT_DIR "${CodePath}/FlatBuffers"
    # CUSTOM_INCLUDE ""
    FLAGS --cpp --gen-mutable --gen-object-api
)

target_link_libraries(Engine PUBLIC EngineGeneratedHeaders)