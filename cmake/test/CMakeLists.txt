cmake_minimum_required(VERSION 3.24)

enable_testing()
include(GoogleTest)

add_subdirectory(memory)

set(TEST_TARGETS 
    TestMemory
)

set(RUN_COMMANDS "")
foreach(target ${TEST_TARGETS})
    list(APPEND RUN_COMMANDS COMMAND $<TARGET_FILE:${target}>)
endforeach()

add_custom_target(AllTests
    ${RUN_COMMANDS}
    COMMENT "Run all unit test..."
    USES_TERMINAL
    VERBATIM
)

add_dependencies(AllTests ${TEST_TARGETS})