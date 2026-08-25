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
project(common_func LANGUAGES C)
add_subdirectory(./at_channel code)

# Full vendor trees are available in opensrc; upstream_github may only ship stubs.
# Skip when building by prebuilt lib, or when vendor CMakeLists.txt is absent.
if(NOT ${BUILD_LIB} STREQUAL "yes")
    if(NOT build_by_lib AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../qualcomm/CMakeLists.txt")
        add_subdirectory(../qualcomm qualcomm_proj)
    endif()
    if(NOT build_by_lib AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../mtk/CMakeLists.txt")
        add_subdirectory(../mtk mtk_proj)
    endif()
endif()

set(CMAKE_C_COMPILER gcc)
set(COMMON_PREPROCESSOR_DEF "-fstack-protector-strong -O2 -Wformat -Wformat-security  -fPIC -std=gnu99")
find_package(LibXml2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 gio-2.0 gio-unix-2.0  udev mm-glib gobject-2.0 dbus-1 mbim-glib)
##################################
# Rolling linux Gdbus codegen
find_program(GDBUSCODEGEN NAMES gdbus-codegen)
if (NOT GDBUSCODEGEN)
    message(SEND_ERROR "Could not find gdbus-codegen")
endif(NOT GDBUSCODEGEN)

add_custom_command(OUTPUT generated/rolling-helper-gdbus-generated.c
    COMMAND ${GDBUSCODEGEN}  --generate-c-code=rolling-helper-gdbus-generated --c-namespace=RollingGdbus --interface-prefix com.rolling. ${CMAKE_SOURCE_DIR}/application/dbus/introspection/com.rolling.helper.xml
    COMMAND mkdir -p generated/
    COMMAND mv rolling-helper-gdbus-generated.c generated/
    )

##################################
# Rolling linux helper service binary configuration
add_definitions(${COMMON_PREPROCESSOR_DEF})
add_compile_options(-pthread)
include_directories(${PROJECT_BINARY_DIR}/)
set(path ${CMAKE_SOURCE_DIR}/application/include/common/  
            ${PROJECT_BINARY_DIR}/../)
include_directories(${path}
                    )

# add different project include path

set(platform_include_dirs ${CMAKE_SOURCE_DIR}/application/rolling_helper_service/)
set(helper_include_dirs ${CMAKE_SOURCE_DIR}/application/rolling_helper_service/common)
include_directories(${platform_include_dirs} ${helper_include_dirs})


include_directories(
                    /usr/include/libmbim-glib
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/../)
set(common_source_files
rolling_helper_basic_func.c
rolling_helper_adapter.c
rolling_helper_log.c
common_base_func.c
generated/rolling-helper-gdbus-generated.c
)

file(GLOB SUBDIRECTORIES "../*")
foreach(SUBDIRS ${SUBDIRECTORIES})
    if(IS_DIRECTORY ${SUBDIRS})
        get_filename_component(SUBDIR_M ${SUBDIRS} NAME)
        if (${SUBDIR_M} STREQUAL "common")
            message(STATUS "Skipping ${SUBDIR_M}")
            continue()
        endif()
        # message(STATUS "platform fund ${SUBDIRS}")

        file(GLOB SUBDIR "${SUBDIRS}/*")
        foreach(SUBDIR_X ${SUBDIR})
            if(IS_DIRECTORY ${SUBDIR_X})
                # message(STATUS "poject fund ${SUBDIR_X}")
                get_filename_component(SUBDIR_N ${SUBDIR_X} NAME)
                file(GLOB SUBDIR_R "${SUBDIR_X}/*")
                foreach(SUBDIR_INFO ${SUBDIR_R})
                    if(IS_DIRECTORY ${SUBDIR_INFO})
                        list(LENGTH SUBDIR_INFO SUBDIR_COUNT)
                        get_filename_component(SUBDIR_NAME ${SUBDIR_INFO} NAME)
                        
                        if (NOT ${BUILD_LIB} STREQUAL "yes")
                            if ((NOT ${PROJECT_BUILD} STREQUAL ${SUBDIR_N}) OR (NOT ${OEM_BUILD} STREQUAL ${SUBDIR_NAME}))
                                if ((NOT ${PROJECT_BUILD} STREQUAL OFF ) AND (NOT ${OEM_BUILD} STREQUAL OFF))
                                    message(STATUS "COMMON Skipping ${SUBDIR_N}_${SUBDIR_NAME}")
                                    continue()
                                endif()
                            endif()
                            message("sub dir: mtk_proj_${SUBDIR_N}_${SUBDIR_NAME}")
                            set(project_include_dirs ${CMAKE_SOURCE_DIR}/application/rolling_helper_service/${SUBDIR_M})
                            set(oem_include_dirs ${CMAKE_SOURCE_DIR}/application/rolling_helper_service/${SUBDIR_M}/${SUBDIR_N})
                            include_directories(${project_include_dirs} ${oem_include_dirs})
                            add_library(common_${SUBDIR_M}_${SUBDIR_N}_${SUBDIR_NAME} STATIC ${common_source_files})
                            target_link_libraries(common_${SUBDIR_M}_${SUBDIR_N}_${SUBDIR_NAME} LibXml2::LibXml2 udev PkgConfig::deps code)
                            target_link_libraries(common_${SUBDIR_M}_${SUBDIR_N}_${SUBDIR_NAME} ${SUBDIR_M}_proj_${SUBDIR_N}_${SUBDIR_NAME})
                        else()
                            if ((NOT ${PROJECT_BUILD} STREQUAL ${SUBDIR_N}) OR (NOT ${OEM_BUILD} STREQUAL ${SUBDIR_NAME}))
                                if ((NOT ${PROJECT_BUILD} STREQUAL OFF ) AND (NOT ${OEM_BUILD} STREQUAL OFF))
                                    message(STATUS "COMMON Skipping ${SUBDIR_N}_${SUBDIR_NAME}")
                                    continue()
                                endif()
                            endif()
                            set(project_include_dirs ${CMAKE_SOURCE_DIR}/application/rolling_helper_service/${SUBDIR_M})
                            set(oem_include_dirs ${CMAKE_SOURCE_DIR}/application/rolling_helper_service/${SUBDIR_M}/${SUBDIR_N})
                            include_directories(${project_include_dirs} ${oem_include_dirs})

                            set(common_lib_source_files rolling_helper_basic_func.c rolling_helper_adapter.c 
                            rolling_helper_log.c common_base_func.c generated/rolling-helper-gdbus-generated.c
                            ${CMAKE_CURRENT_SOURCE_DIR}/../${SUBDIR_M}/rolling_helper_${SUBDIR_M}_common.c
                            ${CMAKE_CURRENT_SOURCE_DIR}/../${SUBDIR_M}/${SUBDIR_N}/rolling_helper_${SUBDIR_N}_common.c
                            ${CMAKE_CURRENT_SOURCE_DIR}/../${SUBDIR_M}/${SUBDIR_N}/${SUBDIR_NAME}/rolling_helper_${SUBDIR_N}_${SUBDIR_NAME}.c
                            )
                            add_library(${SUBDIR_M}_${SUBDIR_N}_${SUBDIR_NAME}_${HELPER_VERSION_STRING} STATIC ${common_lib_source_files})
                            target_link_libraries(${SUBDIR_M}_${SUBDIR_N}_${SUBDIR_NAME}_${HELPER_VERSION_STRING} STATIC LibXml2::LibXml2  udev PkgConfig::deps code)
                        endif()
                    endif()
                endforeach()
            endif()
        endforeach()
    
    endif()
endforeach()


