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

# Note: project version is the rolling-linux version. Update this as per below description
# It is three part.
# <major>.<minor>.<macro>
# major = If any significant design change that improves complete.
# minor = Small design change specific to any operation.
# macro = small change or bug fix

project(rolling_linux VERSION 2.0.29)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PROJECT_BUILD "build project" "")
option(OEM_BUILD "build oem" "")
option(BUILD_LIB "build static lib " "")
option(BUILD_DEB "build deb" "")
option(BUILD_RPM "build rpm" "")
option(BUILD_HELPER "build helper" "")
option(BUILD_DOWN_TOOL "build down tool" "")
option(BUILD_FLASH "build flash" "")
option(BUILD_CONFIG "build config" "")
option(BUILD_BY_LIB "use prebuilt libs" "")

set(build_deb FALSE)
set(build_rpm FALSE)
set(build_by_lib 0)

# 保存命令行传递的值
if(DEFINED BUILD_LIB AND NOT "${BUILD_LIB}" STREQUAL "")
    set(COMMAND_LINE_BUILD_LIB ${BUILD_LIB})
endif()
if(DEFINED BUILD_DEB AND NOT "${BUILD_DEB}" STREQUAL "")
    set(COMMAND_LINE_BUILD_DEB ${BUILD_DEB})
endif()
if(DEFINED BUILD_RPM AND NOT "${BUILD_RPM}" STREQUAL "")
    set(COMMAND_LINE_BUILD_RPM ${BUILD_RPM})
endif()

# 处理BUILD_BY_LIB选项，将其转换为build_by_lib变量
if (BUILD_BY_LIB)
    set(build_by_lib ${BUILD_BY_LIB})
endif()

if (${BUILD_DEB} STREQUAL "yes")
    set(build_deb TRUE)
elseif (${BUILD_RPM} STREQUAL "yes")
    set(build_rpm TRUE)
endif()

# 如果变量为空，使用默认值
if("${PROJECT_BUILD}" STREQUAL "")
    set(PROJECT_BUILD ${DEFAULT_PROJECT_BUILD})
endif()
if("${OEM_BUILD}" STREQUAL "")
    set(OEM_BUILD ${DEFAULT_OEM_BUILD})
endif()

message("start config script!!!")
execute_process(COMMAND bash ${PROJECT_SOURCE_DIR}/cmake_config
                        ${PROJECT_SOURCE_DIR} ${PROJECT_BUILD} ${OEM_BUILD})
message("end config script!!!")

# 检查配置文件是否存在
if(EXISTS ${PROJECT_SOURCE_DIR}/build/${PROJECT_BUILD}_${OEM_BUILD}_config.cmake)
    include(${PROJECT_SOURCE_DIR}/build/${PROJECT_BUILD}_${OEM_BUILD}_config.cmake)
endif()

# 恢复命令行传递的值，如果有的话
if(DEFINED COMMAND_LINE_BUILD_LIB)
    set(BUILD_LIB ${COMMAND_LINE_BUILD_LIB})
endif()
if(DEFINED COMMAND_LINE_BUILD_DEB)
    set(BUILD_DEB ${COMMAND_LINE_BUILD_DEB})
endif()
if(DEFINED COMMAND_LINE_BUILD_RPM)
    set(BUILD_RPM ${COMMAND_LINE_BUILD_RPM})
endif()

# 设置默认值
if("${BUILD_DEB}" STREQUAL "")
    set(BUILD_DEB ${DEFAULT_BUILD_DEB})
endif()
if("${BUILD_RPM}" STREQUAL "")
    set(BUILD_RPM ${DEFAULT_BUILD_RPM})
endif()
if("${BUILD_HELPER}" STREQUAL "")
    set(BUILD_HELPER ${DEFAULT_BUILD_HELPER})
endif()
if("${BUILD_DOWN_TOOL}" STREQUAL "")
    set(BUILD_DOWN_TOOL ${DEFAULT_BUILD_DOWN_TOOL})
endif()
if("${BUILD_FLASH}" STREQUAL "")
    set(BUILD_FLASH ${DEFAULT_BUILD_FLASH})
endif()
if("${BUILD_CONFIG}" STREQUAL "")
    set(BUILD_CONFIG ${DEFAULT_BUILD_CONFIG})
endif()


# 处理BUILD_DEB和BUILD_RPM
if(build_deb)
    set(BUILD_DEB "yes")
    set(BUILD_RPM "no")
elseif(build_rpm)
    set(BUILD_DEB "no")
    set(BUILD_RPM "yes")
endif()

message("build project: ${PROJECT_BUILD}")
message("build oem: ${OEM_BUILD}")
message("build static lib: ${BUILD_LIB}")
message("build deb: ${BUILD_DEB}")
message("build rpm: ${BUILD_RPM}")
message("build helper: ${BUILD_HELPER}")
message("build down tool: ${BUILD_DOWN_TOOL}")
message("build flash: ${BUILD_FLASH}")
message("build config: ${BUILD_CONFIG}")

# 检查是否同时构建deb和rpm
if("${BUILD_DEB}" STREQUAL "yes" AND "${BUILD_RPM}" STREQUAL "yes")
    message(FATAL_ERROR "Can't build both deb and rpm at the same time!")
endif()

add_subdirectory (application)

# enable_testing()
# add_subdirectory(test)

install(CODE "MESSAGE(\"start install script.\")")
install(CODE "execute_process(COMMAND bash ${PROJECT_SOURCE_DIR}/install)")
install(CODE "MESSAGE(\"run install script success.\")")

set(DEFAULT_INSTALL_PREFIX "")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${DEFAULT_INSTALL_PREFIX} CACHE PATH "Default installation path" FORCE)
endif()
message(STATUS "Installation prefix: ${CMAKE_INSTALL_PREFIX}")  
set(install_dir "${PROJECT_BINARY_DIR}/release/dpkg/")

set(outer_loop_exit FALSE)

set(PROJECT_DEB_CONTROL_PATH "${PROJECT_BINARY_DIR}/../script")

set(PROJECT_DEB_CONTROL_PREINST "${PROJECT_DEB_CONTROL_PATH}/preinst")
set(PROJECT_DEB_CONTROL_POSTINST "${PROJECT_DEB_CONTROL_PATH}/postinst")
set(PROJECT_DEB_CONTROL_PRERM "${PROJECT_DEB_CONTROL_PATH}/prerm")
set(PROJECT_DEB_CONTROL_POSTRM "${PROJECT_DEB_CONTROL_PATH}/postrm")

# can only build one deb
file(GLOB SUBDIRECTORIES "${CMAKE_SOURCE_DIR}/application/rolling_helper_service/*")
foreach(SUBDIRS ${SUBDIRECTORIES})
    if(IS_DIRECTORY ${SUBDIRS})
        get_filename_component(SUBDIR_M ${SUBDIRS} NAME)
        if (${SUBDIR_M} STREQUAL "common")
            message(STATUS "Skipping ${SUBDIR_M}")
            continue()
        endif()
        
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
                                message(STATUS "Skipping ${SUBDIR_N}_${SUBDIR_NAME}")
                                continue()
                            endif()
                        endif()

                        # if ((${PROJECT_BUILD} STREQUAL ${SUBDIR_N}) AND (${OEM_BUILD} STREQUAL ${SUBDIR_NAME}))
                        list(LENGTH SUBDIR_INFO SUBDIR_COUNT)
                        
                        set(project_path ${SUBDIR_M}_${SUBDIR_N}_${SUBDIR_NAME})
                        message(STATUS " INSTALL project_path ${project_path}")
                        set(install_dir ${PROJECT_BINARY_DIR}/release/dpkg/${project_path})

                        # 使用install脚本创建的目录结构
                        set(deb_soft_dirs "${PROJECT_BINARY_DIR}/release/dpkg")
                        
                        # 安装opt目录
                        install(DIRECTORY "${deb_soft_dirs}/opt/rolling" 
                                FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ 
                                DESTINATION "${CMAKE_INSTALL_PREFIX}/opt")
                        
                        # 安装udev相关目录
                        install(DIRECTORY "${deb_soft_dirs}/usr/lib/udev/"  
                                FILE_PERMISSIONS    
                                OWNER_EXECUTE OWNER_WRITE OWNER_READ
                                GROUP_EXECUTE GROUP_READ 
                                WORLD_EXECUTE WORLD_READ
                                DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/lib/udev/")
                        
                        install(DIRECTORY "${deb_soft_dirs}/usr/lib/udev/rules.d/"  
                                FILE_PERMISSIONS    
                                OWNER_EXECUTE OWNER_WRITE OWNER_READ
                                GROUP_EXECUTE GROUP_READ 
                                WORLD_EXECUTE WORLD_READ
                                DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/lib/udev/rules.d")
                        
                        # 安装systemd服务目录
                        install(DIRECTORY "${deb_soft_dirs}/lib/systemd/system/"  
                                FILE_PERMISSIONS    
                                OWNER_EXECUTE OWNER_WRITE OWNER_READ
                                GROUP_EXECUTE GROUP_READ 
                                WORLD_EXECUTE WORLD_READ
                                DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/systemd/system")
                        
                        # 安装dbus配置目录
                        install(DIRECTORY "${deb_soft_dirs}/usr/share/dbus-1/"  
                                FILE_PERMISSIONS    
                                OWNER_EXECUTE OWNER_WRITE OWNER_READ
                                GROUP_EXECUTE GROUP_READ 
                                WORLD_EXECUTE WORLD_READ
                                DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/share/dbus-1/")
                        
                        install(DIRECTORY "${deb_soft_dirs}/usr/share/dbus-1/system.d/"  
                                FILE_PERMISSIONS    
                                OWNER_EXECUTE OWNER_WRITE OWNER_READ
                                GROUP_EXECUTE GROUP_READ 
                                WORLD_EXECUTE WORLD_READ
                                DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/share/dbus-1/system.d")
                        install(CODE "MESSAGE(\"install success!\")")
                        
                        set(CPACK_PACKAGE_NAME "linux-apps-${SUBDIR_M}-${SUBDIR_N}-${SUBDIR_NAME}")
                        set(CPACK_PACKAGE_VENDOR "rolling")

                        if (${BUILD_DEB} STREQUAL "yes")
                            set(CPACK_GENERATOR "DEB")
                            set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
                            set(CPACK_DEBIAN_PACKAGE_GROUP "rolling")
                            set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "WWAN Linux apps")
                            set(CPACK_DEBIAN_PACKAGE_SECTION "base")
                            set(CPACK_DEBIAN_PACKAGE_PRIORITY "standard")
                            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
                            set(CPACK_DEBIAN_PACKAGE_MAINTAINER "rolling")
                            set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA 
                                "${PROJECT_DEB_CONTROL_PREINST};${PROJECT_DEB_CONTROL_POSTINST};${PROJECT_DEB_CONTROL_PRERM};${PROJECT_DEB_CONTROL_POSTRM}")
                        elseif (${BUILD_RPM} STREQUAL "yes")
                            set(CPACK_GENERATOR "RPM")
                            # set(CMAKE_BUILD_TYPE Release)
                            set(CPACK_STRIP_FILES TRUE)
                            set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
                            set(CPACK_RPM_PACKAGE_SUMMARY "WWAN Linux apps")
                            set(CPACK_RPM_PACKAGE_DEBUG FALSE)
                            set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
                            set(CPACK_RPM_PACKAGE_GROUP "Applications/Internet")
                            set(CPACK_RPM_PACKAGE_DESCRIPTION "WWAN Linux apps")
                            set(CPACK_RPM_PACKAGE_LICENSE "GPL")
                            set(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE "${PROJECT_DEB_CONTROL_PREINST}")
                            set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${PROJECT_DEB_CONTROL_POSTINST}")
                            set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${PROJECT_DEB_CONTROL_PRERM}")
                            set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${PROJECT_DEB_CONTROL_POSTRM}")
                            set(CPACK_RPM_RELOCATION_PATHS "/lib/systemd/system;/opt/rolling")
                        else()
                            set(CPACK_GENERATOR "DEB")
                            set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
                            set(CPACK_DEBIAN_PACKAGE_GROUP "rolling")
                            set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "WWAN Linux apps")
                            set(CPACK_DEBIAN_PACKAGE_SECTION "base")
                            set(CPACK_DEBIAN_PACKAGE_PRIORITY "standard")
                            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
                            set(CPACK_DEBIAN_PACKAGE_MAINTAINER "rolling")
                            set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA 
                                "${PROJECT_DEB_CONTROL_PREINST};${PROJECT_DEB_CONTROL_POSTINST};${PROJECT_DEB_CONTROL_PRERM};${PROJECT_DEB_CONTROL_POSTRM}")
                        endif()
                        set(outer_loop_exit TRUE)
                        break()
                    endif()
                endforeach()
            endif()
            if(outer_loop_exit)
                break()
            endif()
        endforeach()
    endif()
    if(outer_loop_exit)
        break()
    endif()
endforeach()

# set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/CPackConfig_qualcomm_rw135_lenovo.cmake")

include(CPack)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
  @ONLY
)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
