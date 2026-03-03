function(fe_link_engine_sdks TARGET_NAME)
    find_package(directx-headers CONFIG REQUIRED)
    find_package(taskflow CONFIG REQUIRED)
    find_package(sol2 CONFIG REQUIRED)
    find_package(assimp CONFIG REQUIRED)
    find_package(glm CONFIG REQUIRED)
    find_package(fmt CONFIG REQUIRED)
    find_package(EnTT CONFIG REQUIRED)
    find_package(mimalloc CONFIG REQUIRED)
    find_package(EASTL CONFIG REQUIRED)
    find_package(flatbuffers CONFIG REQUIRED)
    find_package(winpixevent CONFIG REQUIRED)
    find_package(benchmark CONFIG REQUIRED)
    find_package(directx12-agility CONFIG REQUIRED)
    find_package(PkgConfig REQUIRED)
    find_package(cereal CONFIG REQUIRED)
    pkg_check_modules(LUAJIT REQUIRED IMPORTED_TARGET luajit)

    target_link_libraries(${TARGET_NAME} PUBLIC
        assimp::assimp
        glm::glm
        fmt::fmt
        EnTT::EnTT
        mimalloc
        EASTL
        flatbuffers::flatbuffers

        Microsoft::WinPixEventRuntime
        Microsoft::DirectX-Headers
        Microsoft::DirectX-Guids
        Microsoft::DirectX12-Agility

        PkgConfig::LUAJIT

        benchmark::benchmark
        benchmark::benchmark_main
        
        cereal::cereal
    )

    if(TARGET Microsoft::DirectX12-Agility)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET_NAME}>/D3D12"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Microsoft::DirectX12-Core>" "$<TARGET_FILE_DIR:${TARGET_NAME}>/D3D12/"
            COMMAND $<$<CONFIG:Debug>:${CMAKE_COMMAND}> $<$<CONFIG:Debug>:-E> $<$<CONFIG:Debug>:copy_if_different> 
                    "$<TARGET_FILE:Microsoft::DirectX12-Layers>" 
                    "$<TARGET_FILE_DIR:${TARGET_NAME}>/D3D12/"
            COMMENT "Syncing D3D12 Agility SDK binaries for ${TARGET_NAME}..."
        )
    endif()

    if(NOT DX12_Agility_VERSION)
        message(FATAL_ERROR "DX12_Agility_VERSION not defined!")
    endif()
    message(STATUS "Agility SDK Version defined from Preset: ${DX12_Agility_VERSION}")
    target_compile_definitions(${TARGET_NAME} PUBLIC RE_D3D12_AGILITY_SDK_VERSION=${DX12_Agility_VERSION})

    set(SDK_INCLUDE_PATH "${SDK_ROOT}/include")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(SDK_BIN_PATH "${SDK_ROOT}/debug/bin")
        set(SDK_LIB_PATH "${SDK_ROOT}/debug/lib")
    else()
        set(SDK_BIN_PATH "${SDK_ROOT}/bin")
        set(SDK_LIB_PATH "${SDK_ROOT}/lib")
    endif()

    file(GLOB_RECURSE EXTERNAL_LIBS
        "${SDK_LIB_PATH}/alembic-md.lib"
        "${SDK_LIB_PATH}/libfbxsdk-md.lib"
        "${SDK_LIB_PATH}/libxml2-md.lib"
        "${SDK_LIB_PATH}/zlib-md.lib"
    )

    target_include_directories(${TARGET_NAME} PUBLIC ${SDK_INCLUDE_PATH})
    target_link_libraries(${TARGET_NAME} PRIVATE ${EXTERNAL_LIBS})

    target_link_libraries(${TARGET_NAME} PUBLIC
        d3d12.lib
        dxgi.lib
        d3dcompiler.lib
        dxguid.lib
    )
endfunction()