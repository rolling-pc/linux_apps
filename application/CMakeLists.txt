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

if(NOT ${BUILD_LIB} STREQUAL "yes")
    add_subdirectory(3rd)
    if (${BUILD_FLASH} STREQUAL "yes")
        add_subdirectory(rolling_flash_service)
    endif()
    if (${BUILD_CONFIG} STREQUAL "yes")
        add_subdirectory(rolling_config_service)
    endif()
endif()


if(${BUILD_LIB} STREQUAL "yes")
    add_subdirectory(3rd)
    if (${BUILD_FLASH} STREQUAL "yes")
        add_subdirectory(rolling_flash_service) 
    endif()
    if (${BUILD_CONFIG} STREQUAL "yes")
        add_subdirectory(rolling_config_service)
    endif()
endif()

if (${BUILD_HELPER} STREQUAL "yes")
    add_subdirectory(rolling_helper_service)
endif()