#/* This program is free software; you can redistribute it and/or modify
# * it under the terms of the GNU Lesser General Public License as published by
# * the Free Software Foundation; either version 2 of the License, or
# * (at your option) any later version.
# *
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU Lesser General Public License for more details.
# *
# * You should have received a copy of the GNU Lesser General Public License
# * along with this program. If not, see <http://www.gnu.org/licenses/>.
# *
# * Copyright (C) 2025, Rolling Wireless S.a.r.l.
# */
cmake_minimum_required(VERSION 3.6)
project(rolling_helper_service)

set(HELPER_MAJOR_VERSION 2)
set(HELPER_MINOR_VERSION 0)
set(HELPER_PATCH_VERSION 25)
set(HELPER_VERSION_STRING ${HELPER_MAJOR_VERSION}.${HELPER_MINOR_VERSION}.${HELPER_PATCH_VERSION})
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
                "./version.h")

set(CMAKE_C_COMPILER gcc)
set(HELPER_PREPROCESSOR_DEF "-fstack-protector-strong -O2 -Wformat -Wformat-security  -fPIC -std=gnu99")
find_package(PkgConfig REQUIRED)
find_package(LibXml2 REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 gio-2.0 gio-unix-2.0 mbim-glib mm-glib)
##################################

##################################
# Rolling linux helper service binary configuration
add_compile_options(-pthread)
set(path ${CMAKE_SOURCE_DIR}/application/include/common/ 
${PROJECT_BINARY_DIR}/ ${CMAKE_CURRENT_SOURCE_DIR}/common 
${PROJECT_BINARY_DIR}/common)
include_directories(${path})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include
                    /usr/include/libmbim-glib
                    /usr/local/include/libmbim-glib
                    ${CMAKE_CURRENT_SOURCE_DIR})

set(helper_source_files rolling_helper_main.c)

set(build_by_code 1)

# Resolve BUILD_BY_LIB before deciding whether to add ./common (upstream may lack vendor trees).
if(DEFINED BUILD_BY_LIB)
    set(build_by_lib ${BUILD_BY_LIB})
    message(STATUS "BUILD_BY_LIB parameter: ${BUILD_BY_LIB}")
endif()

file(GLOB HELPER_DIR "./*")
foreach(SUBDIRS ${HELPER_DIR})
    get_filename_component(SUBDIR_M ${SUBDIRS} NAME)
    if (${SUBDIR_M} STREQUAL "common")
        # 只有当build_by_lib为0时，才添加common子目录
        if(NOT build_by_lib)
            add_subdirectory(./common)
        endif()
        break()
    elseif(${SUBDIR_M} STREQUAL "common_lib")
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common_lib)
        break()
    endif()
endforeach()

# 当BUILD_LIB为yes时，自动设置build_by_code为1
if(${BUILD_LIB} STREQUAL "yes")
    set(build_by_code 1)
    message(STATUS "Build lib mode enabled, setting build_by_code=1")
else()
    message(STATUS "Build exe mode enabled, setting build_by_code=0")
    # 只有在非BUILD_LIB模式下，当build_by_lib为1时，才设置BUILD_LIB为no
    if(build_by_lib)
        set(build_by_code 0)
        set(BUILD_LIB "no")
        message(STATUS "Build by lib mode enabled, setting BUILD_LIB=no")
    endif()
endif()

add_definitions(${HELPER_PREPROCESSOR_DEF})

# 定义通用链接库变量
set(HELPER_COMMON_LIBS
    PkgConfig::deps
    LibXml2::LibXml2
    udev
    stdc++
    stdc++fs
)

# 定义通用链接库变量（带safestring）
set(HELPER_COMMON_LIBS_WITH_SAFESTRING
    ${PROJECT_BINARY_DIR}/../3rd/safestringlib/libsafestring_static.a
    ${HELPER_COMMON_LIBS}
)

# 创建链接库的函数，封装重复的链接逻辑
function(helper_link_libraries target_name)
    # 链接库，传递所有额外参数作为库
    target_link_libraries(${target_name}
        ${ARGN}
        ${HELPER_COMMON_LIBS_WITH_SAFESTRING}
    )
endfunction()

# D-Bus 接口生成
find_program(GDBUSCODEGEN NAMES gdbus-codegen)
if (NOT GDBUSCODEGEN)
    message(SEND_ERROR "Could not find gdbus-codegen")
endif(NOT GDBUSCODEGEN)

add_custom_command(
    OUTPUT generated/rolling-helper-gdbus-generated.c generated/rolling-helper-gdbus-generated.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/generated
    COMMAND ${GDBUSCODEGEN}  --generate-c-code=${CMAKE_CURRENT_BINARY_DIR}/generated/rolling-helper-gdbus-generated --c-namespace=RollingGdbus --interface-prefix com.rolling. ${CMAKE_SOURCE_DIR}/application/dbus/introspection/com.rolling.helper.xml
    )

# 创建一个自定义目标来确保D-Bus接口文件被生成
add_custom_target(helper_dbus_generated ALL DEPENDS generated/rolling-helper-gdbus-generated.c generated/rolling-helper-gdbus-generated.h)

# 添加生成目录到包含路径
include_directories(${CMAKE_CURRENT_BINARY_DIR}/generated)

# 根据构建模式决定输出目标
# 优先检查build_by_lib模式，从预构建静态库生成可执行文件
if(build_by_lib)
    # 当build_by_lib=1时，跳过源码编译，只从库文件构建
    message(STATUS "STATTUSbuild lib not build exe")

    # 静态库路径指向binary_deb/common_lib目录下的通用库文件
    if(PROJECT_BUILD STREQUAL "rw350r")
        set(HELPER_PLATFORM "mtk")
    else()
        set(HELPER_PLATFORM "qualcomm")
    endif()
    set(STATIC_LIB_PATH "${CMAKE_SOURCE_DIR}/binary_deb/common_lib/lib${HELPER_PLATFORM}_${PROJECT_BUILD}_${OEM_BUILD}_${HELPER_VERSION_STRING}.a")
    message(STATUS "Using static library: ${STATIC_LIB_PATH}")

    # 构建通用可执行文件，使用实际的rolling_helper_main.c文件
    add_executable(rolling_helper ${CMAKE_CURRENT_SOURCE_DIR}/rolling_helper_main.c)
    # 链接静态库
    helper_link_libraries(rolling_helper "${STATIC_LIB_PATH}" "${CMAKE_SOURCE_DIR}/binary_deb/common_lib/libcode.a")
    message(STATUS "Building generic executable using library: rolling_helper")
# 然后检查BUILD_LIB模式，构建静态库，但只有当build_by_lib不为1时才执行
elseif (BUILD_LIB AND NOT build_by_lib)
    # 构建静态库模式
    message(STATUS "Building helper service as static library")

    # 收集所有源文件（包括common目录下的源文件和生成的D-Bus文件）
    file(GLOB COMMON_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/common/*.c")
    # 构建静态库时不包含rolling_helper_main.c，它将在生成可执行文件时单独使用
    set(ALL_SOURCE_FILES 
        ${COMMON_SOURCE_FILES}
        ${CMAKE_CURRENT_BINARY_DIR}/generated/rolling-helper-gdbus-generated.c
    )

    add_library(rolling_helper_service_${HELPER_VERSION_STRING} STATIC ${ALL_SOURCE_FILES})
    # 确保静态库构建依赖于D-Bus生成目标
    add_dependencies(rolling_helper_service_${HELPER_VERSION_STRING} helper_dbus_generated)
    helper_link_libraries(rolling_helper_service_${HELPER_VERSION_STRING})

    # 安装静态库
    install(TARGETS rolling_helper_service_${HELPER_VERSION_STRING} DESTINATION lib)
    install(FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/common/rolling_helper_common.h 
        ${CMAKE_CURRENT_SOURCE_DIR}/common/rolling_helper_basic_func.h 
        ${CMAKE_CURRENT_SOURCE_DIR}/common/common_base_func.h 
        DESTINATION include)

# 最后检查常规可执行文件构建模式
elseif(NOT build_by_lib AND NOT BUILD_LIB)
    # 构建可执行文件模式
    message(STATUS "Building helper service as executable")

    # 常规构建模式，遍历目录结构
    file(GLOB SUBDIRECTORIES "./*")
    foreach(SUBDIRS ${SUBDIRECTORIES})
        if(IS_DIRECTORY ${SUBDIRS})
            get_filename_component(SUBDIR_M ${SUBDIRS} NAME)
            if (${SUBDIR_M} STREQUAL "common")
                set(build_by_code 1)
                message(STATUS "Skipping ${SUBDIR_M}")
                continue()
            endif()
            if ((${SUBDIR_M} STREQUAL "common_lib") AND (NOT build_by_code))
                set(build_by_lib 1)
                file(GLOB file_names "${SUBDIR_M}/*.a")
                foreach(file ${file_names})
                    get_filename_component(lib_name ${file} NAME)
                    if ((NOT ${PROJECT_BUILD} STREQUAL OFF ) AND (NOT ${OEM_BUILD} STREQUAL OFF))
                        set(pattern ".*${PROJECT_BUILD}_${OEM_BUILD}.*")
                        string(REGEX MATCH "${pattern}" is_matched "${lib_name}")
                        if(is_matched)
                            string(LENGTH "${lib_name}" string_length)
                            set(start_index 3)
                            math(EXPR end_index "${string_length} - 11") 
                            string(SUBSTRING "${lib_name}" ${start_index} ${end_index} exe_prefix)
                            message("executable lib:${exe_prefix}")
                            add_executable(rolling_helper ${helper_source_files})
                            helper_link_libraries(rolling_helper "${file}" "${CMAKE_CURRENT_SOURCE_DIR}/common_lib/libcode.a")
                        endif()
                    else()
                        set(PROJECT_BUILD rw101 rw135 rw350r)
                        set(OEM_BUILD dell lenovo hp)
                        foreach(project ${PROJECT_BUILD})
                            foreach(oem ${OEM_BUILD})
                                set(pattern ".*${project}_${oem}.*")
                                string(REGEX MATCH "${pattern}" is_matched "${lib_name}")
                                if(is_matched)
                                    string(LENGTH "${lib_name}" string_length)
                                    set(start_index 3)
                                    math(EXPR end_index "${string_length} - 11") 
                                    string(SUBSTRING "${lib_name}" ${start_index} ${end_index} exe_prefix)
                                    message("executable lib:${exe_prefix}")
                                    add_executable(${exe_prefix}_rolling_helper ${helper_source_files})
                                    helper_link_libraries(${exe_prefix}_rolling_helper "${file}" "${CMAKE_CURRENT_SOURCE_DIR}/common_lib/libcode.a")
                                endif()
                            endforeach()
                        endforeach()
                        set(PROJECT_BUILD )
                        set(OEM_BUILD )
                    endif()
                endforeach()
                break()
            else()
                file(GLOB SUBDIR "${SUBDIRS}/*")
                foreach(SUBDIR_X ${SUBDIR})
                    if(IS_DIRECTORY ${SUBDIR_X})
                        get_filename_component(SUBDIR_N ${SUBDIR_X} NAME)
                        file(GLOB SUBDIR_R "${SUBDIR_X}/*")
                        foreach(SUBDIR_INFO ${SUBDIR_R})
                            if(IS_DIRECTORY ${SUBDIR_INFO})
                                get_filename_component(SUBDIR_NAME ${SUBDIR_INFO} NAME)
                                if ((NOT ${PROJECT_BUILD} STREQUAL ${SUBDIR_N}) OR (NOT ${OEM_BUILD} STREQUAL ${SUBDIR_NAME}))
                                    if ((NOT ${PROJECT_BUILD} STREQUAL OFF ) AND (NOT ${OEM_BUILD} STREQUAL OFF))
                                        message(STATUS "EXEC Skipping ${SUBDIR_N}_${SUBDIR_NAME}")
                                        continue()
                                    endif()
                                endif()
                                message("executable lib: ${SUBDIR_M}_proj_${SUBDIR_N}_${SUBDIR_NAME}")
                                add_executable(rolling_helper ${helper_source_files})
                                # 注意：这里不使用helper_link_libraries函数，因为它依赖于common_${SUBDIR_M}_${SUBDIR_N}_${SUBDIR_NAME}，
                                # 而这个目标是在common/CMakeLists.txt中定义的，不在当前函数的作用域内
                                target_link_libraries(rolling_helper LibXml2::LibXml2 PkgConfig::deps code 
                                                    common_${SUBDIR_M}_${SUBDIR_N}_${SUBDIR_NAME})
                            endif()
                        endforeach()
                    endif()
                endforeach()
            endif()
        endif()
    endforeach()
endif()

if (build_by_code AND (${BUILD_LIB} STREQUAL "yes"))
    set(lib_path ${PROJECT_BINARY_DIR}/../../common_lib)
    if ((${PROJECT_BUILD} STREQUAL "rw101") OR (${PROJECT_BUILD} STREQUAL "rw135"))
        add_custom_target(
            helper_build_lib
            COMMAND mkdir -p ${lib_path}
            COMMAND cp  -raf ${PROJECT_BINARY_DIR}/common/libqualcomm*.a ${lib_path}
            COMMAND cp  -raf ${PROJECT_BINARY_DIR}/common/code/lib*.a ${lib_path}
            COMMAND cp  -raf ${CMAKE_CURRENT_SOURCE_DIR}/common/rolling_helper_common.h ${lib_path}
            COMMAND cp  -raf ${CMAKE_CURRENT_SOURCE_DIR}/common/rolling_helper_basic_func.h ${lib_path}
            COMMAND cp  -raf ${CMAKE_CURRENT_SOURCE_DIR}/common/common_base_func.h ${lib_path}
            COMMAND cp  -raf ${PROJECT_BINARY_DIR}/common/rolling-helper-gdbus-generated.h ${lib_path}
            COMMENT "helper service build lib successfully"
        )
    elseif(${PROJECT_BUILD} STREQUAL "rw350r")
        add_custom_target(
            helper_build_lib
            COMMAND mkdir -p ${lib_path}
            COMMAND cp  -raf ${PROJECT_BINARY_DIR}/common/libmtk*.a ${lib_path}
            COMMAND cp  -raf ${PROJECT_BINARY_DIR}/common/code/lib*.a ${lib_path}
            COMMAND cp  -raf ${CMAKE_CURRENT_SOURCE_DIR}/common/rolling_helper_common.h ${lib_path}
            COMMAND cp  -raf ${CMAKE_CURRENT_SOURCE_DIR}/common/rolling_helper_basic_func.h ${lib_path}
            COMMAND cp  -raf ${CMAKE_CURRENT_SOURCE_DIR}/common/common_base_func.h ${lib_path}
            COMMAND cp  -raf ${PROJECT_BINARY_DIR}/common/rolling-helper-gdbus-generated.h ${lib_path}
            COMMENT "helper service build lib successfully"
        )
    else()
        add_custom_target(
            helper_build_lib
            COMMAND mkdir -p ${lib_path}
            COMMAND cp  -raf ${PROJECT_BINARY_DIR}/common/libqualcomm*.a ${lib_path}
            COMMAND cp  -raf ${PROJECT_BINARY_DIR}/common/libmtk*.a ${lib_path}
            COMMAND cp  -raf ${PROJECT_BINARY_DIR}/common/code/lib*.a ${lib_path}
            COMMAND cp  -raf ${CMAKE_CURRENT_SOURCE_DIR}/common/rolling_helper_common.h ${lib_path}
            COMMAND cp  -raf ${CMAKE_CURRENT_SOURCE_DIR}/common/rolling_helper_basic_func.h ${lib_path}
            COMMAND cp  -raf ${CMAKE_CURRENT_SOURCE_DIR}/common/common_base_func.h ${lib_path}
            COMMAND cp  -raf ${PROJECT_BINARY_DIR}/common/rolling-helper-gdbus-generated.h ${lib_path}
            COMMENT "helper service build lib successfully"
        )
    endif()
endif()
