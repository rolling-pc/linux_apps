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

project(generic_func LANGUAGES C)

set(CMAKE_C_COMPILER gcc)
set(COMMON_PREPROCESSOR_DEF "-fstack-protector-strong -O2 -Wformat -Wformat-security  -fPIC -std=gnu99")

find_package(LibXml2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 gio-2.0 gio-unix-2.0 udev mm-glib gobject-2.0 dbus-1)

##################################
# Rolling linux helper service binary configuration

add_definitions(${COMMON_PREPROCESSOR_DEF})

set(platform_include_dirs ${CMAKE_SOURCE_DIR}/application/rolling_helper_service/)
set(project_include_dirs ${CMAKE_SOURCE_DIR}/application/rolling_helper_service/mtk)
set(oem_include_dirs ${CMAKE_SOURCE_DIR}/application/rolling_helper_service/mtk/rw350)
set(helper_include_dirs ${CMAKE_SOURCE_DIR}/application/rolling_helper_service/common)

add_compile_options(-pthread)
include_directories(${PROJECT_BINARY_DIR}/  ${PROJECT_BINARY_DIR}/generic_func ${platform_include_dirs} ${project_include_dirs} ${oem_include_dirs} ${helper_include_dirs})
set(path ${CMAKE_SOURCE_DIR}/application/include/common/  ${PROJECT_BINARY_DIR}/../)
include_directories(${path}
                    )
include_directories(
                    /usr/include/libmbim-glib
                    /usr/local/include/libmbim-glib/
                    ${CMAKE_CURRENT_SOURCE_DIR}/../)

set(generic_source_files rolling_helper_rw350_generic.c)

add_library(generic_func_rw350 STATIC ${generic_source_files})

target_link_libraries(generic_func_rw350 libmbim-glib.so udev LibXml2::LibXml2 PkgConfig::deps)
