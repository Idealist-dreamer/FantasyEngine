
message(STATUS "====Check SDK Begin====")

# vcpkg info
message(STATUS "VCPKG_ROOT: ${VCPKG_ROOT}")
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")
message(STATUS "VCPKG_INSTALL_PATH: ${VCPKG_INSTALL_PATH}")

# qt info
message(STATUS "QT_ROOT: ${QT_ROOT}")
message(STATUS "QT_VERSION_MAJOR: ${QT_VERSION_MAJOR}")

# sdk info
message(STATUS "SDK_ROOT: ${SDK_ROOT}")

# check valid
function(fe_check_path_exit pathName)
    if(DEFINED ${pathName})
        if(EXISTS "${${pathName}}")
        else()
            message(WARNING "Path not found, ${pathName} : ${${pathName}}")
        endif()
    else()
        message(WARNING "Path variable not defined : ${pathName}")
    endif()
endfunction()

function(CheckFileExist fileName)
    if(DEFINED ${fileName})
        if(EXISTS "${${fileName}}" AND NOT IS_DIRECTORY "${${fileName}}")
        else()
            message(WARNING "File not found, ${fileName} : ${${fileName}}")
        endif()
    else()
        message(WARNING "File variable not defined : ${fileName}")
    endif()
endfunction()

fe_check_path_exit(VCPKG_ROOT)
fe_check_path_exit(VCPKG_INSTALL_PATH)
fe_check_path_exit(CMAKE_TOOLCHAIN_FILE)
fe_check_path_exit(QT_ROOT)
fe_check_path_exit(SDK_ROOT)

message(STATUS "====Check SDK Valid End====")