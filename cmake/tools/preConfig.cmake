set(VCPKG_INSTALL_PATH "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}")

if(MSVC)
    add_compile_options(/EHsc /Qspectre)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
    
    # Release build optimizations
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2 /GL /Gy)
        string(APPEND CMAKE_EXE_LINKER_FLAGS " /LTCG /OPT:REF /OPT:ICF")
        string(APPEND CMAKE_SHARED_LINKER_FLAGS " /LTCG /OPT:REF /OPT:ICF")
    endif()
endif()

function(fe_set_output_directory target)
    set(OUTPUT_BASE_DIR "${RUNTIME_ARCHIVE_LIBRARY_BASE_DIRECTORY}")
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/${target}/Bin"
        ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/${target}/Lib"
        LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/${target}/Lib"
    )
endfunction()