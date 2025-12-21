# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# 该文件复制自 Qt 6.9.0，移除了 macOS Tahoe 不再存在的 AGL.framework 依赖，避免链接失败
message(STATUS "WrapOpenGL finder from project/cmake/FindWrapOpenGL.cmake 已启用")

if(TARGET WrapOpenGL::WrapOpenGL)
    set(WrapOpenGL_FOUND ON)
    return()
endif()

set(WrapOpenGL_FOUND OFF)

find_package(OpenGL ${WrapOpenGL_FIND_VERSION})

if (OpenGL_FOUND)
    set(WrapOpenGL_FOUND ON)

    add_library(WrapOpenGL::WrapOpenGL INTERFACE IMPORTED)
    if(APPLE)
        get_target_property(__opengl_fw_lib_path OpenGL::GL IMPORTED_LOCATION)
        if(__opengl_fw_lib_path AND NOT __opengl_fw_lib_path MATCHES "/([^/]+)\\.framework$")
            get_filename_component(__opengl_fw_path "${__opengl_fw_lib_path}" DIRECTORY)
        endif()

        if(NOT __opengl_fw_path)
            set(__opengl_fw_path "-framework OpenGL")
        endif()

        target_link_libraries(WrapOpenGL::WrapOpenGL INTERFACE ${__opengl_fw_path})
        # 直接跳过 AGL.framework，避免在 macOS 14+ 链接阶段报错
        message(STATUS "WrapOpenGL: macOS 上跳过 AGL.framework 链接以兼容 Tahoe 及更新系统")
    else()
        target_link_libraries(WrapOpenGL::WrapOpenGL INTERFACE OpenGL::GL)
    endif()
elseif(UNIX AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "Integrity")
    find_package(OpenGL ${WrapOpenGL_FIND_VERSION} COMPONENTS OpenGL)
    if (OpenGL_FOUND)
        set(WrapOpenGL_FOUND ON)
        add_library(WrapOpenGL::WrapOpenGL INTERFACE IMPORTED)
        target_link_libraries(WrapOpenGL::WrapOpenGL INTERFACE OpenGL::OpenGL)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapOpenGL DEFAULT_MSG WrapOpenGL_FOUND)
