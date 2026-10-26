#/* This program is free software: you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation, either version 2 of the License, or
# * (at your option) any later version.
# *
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License
# * along with this program.  If not, see <http://www.gnu.org/licenses/>.
# *
# * Copyright (C) 2023, Rolling Wireless S.a.r.l.
# */

cmake_minimum_required(VERSION 3.10)

project(3rd)

add_subdirectory(iniparser)
add_subdirectory(safestringlib)
if (${BUILD_DOWN_TOOL} STREQUAL "yes")
    add_subdirectory(qdl)
endif()

    