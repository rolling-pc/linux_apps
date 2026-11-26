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
# * Copyright (C) 2025-2026, Rolling Wireless S.a.r.l.
# */

cmake_minimum_required(VERSION 3.10)

# Note: project version is the rolling-linux version. Update this as per below description
# It is three part.
# <major>.<minor>.<macro>
# major = If any significant design change that improves complete.
# minor = Small design change specific to any operation.
# macro = small change or bug fix

project(rolling_linux VERSION 2.1.11)
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
option(BUILD_MA "build ma" "")
option(BUILD_MA_LIB "build ma lib artifact" "")
option(BUILD_BY_LIB "use prebuilt libs" "")
set(DISTRO_TAG "ubuntu26.04" CACHE STRING "Debian package release tag; affects the prebuilt rolling_ma lookup path only. Third-party libs are resolved from the system via the declared package dependencies (no bundling), so the package runs on Ubuntu 26.04 and later; no per-tag gating is enforced.")

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
if("${BUILD_MA}" STREQUAL "")
    set(BUILD_MA ${DEFAULT_BUILD_MA})
endif()
if("${BUILD_MA_LIB}" STREQUAL "")
    set(BUILD_MA_LIB ${DEFAULT_BUILD_MA_LIB})
endif()
if("${BUILD_MA_LIB}" STREQUAL "" AND "${BUILD_MA}" STREQUAL "yes")
    set(BUILD_MA_LIB "yes")
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
message("build ma: ${BUILD_MA}")
message("build ma lib: ${BUILD_MA_LIB}")

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

# NOTE: do not force CMAKE_INSTALL_PREFIX here.
#
# The FORCEd CACHE set that used to live here overwrote whatever prefix was
# passed on the command line, so "cmake --install" wrote into the host root and
# debhelper found debian/tmp empty (dh_install then failed on every path).
#
# The prefix is owned by the caller:
#   - debhelper : debian/rules passes -DCMAKE_INSTALL_PREFIX=/ and dh supplies
#                 DESTDIR=debian/tmp, so the relative DESTINATIONs below resolve
#                 to debian/tmp/usr/bin, debian/tmp/usr/share/linux-apps, etc.
#   - CPack     : the caller passes no prefix; CPACK_SET_DESTDIR/CPack staging
#                 handles it.  The destinations must stay relative for CPack.
message(STATUS "Installation prefix: ${CMAKE_INSTALL_PREFIX}")
set(install_dir "${PROJECT_BINARY_DIR}/release/dpkg/")

# A single set of maintainer scripts serves the universal package (ships the
# FCC unlock backends for every OEM; the fcc-unlock scripts dispatch on the
# DMI vendor at runtime).  The scripts under script/ guard every MA-specific
# action with an existence check.  There is no separate no_ma_script/ tree.
set(PROJECT_DEB_CONTROL_PATH "${PROJECT_SOURCE_DIR}/script")
message(STATUS "Debian control scripts: ${PROJECT_DEB_CONTROL_PATH} (BUILD_MA=${BUILD_MA})")

set(PROJECT_DEB_CONTROL_PREINST "${PROJECT_DEB_CONTROL_PATH}/preinst")
set(PROJECT_DEB_CONTROL_POSTINST "${PROJECT_DEB_CONTROL_PATH}/postinst")
set(PROJECT_DEB_CONTROL_PRERM "${PROJECT_DEB_CONTROL_PATH}/prerm")
set(PROJECT_DEB_CONTROL_POSTRM "${PROJECT_DEB_CONTROL_PATH}/postrm")

macro(rolling_linux_configure_deb_package)
    if(DISTRO_TAG)
        set(CPACK_DEBIAN_PACKAGE_RELEASE "${DISTRO_TAG}")
        message(STATUS "Debian package release tag: ${DISTRO_TAG}")
    endif()

    # The CPack DEB generator defaults CPACK_PACKAGING_INSTALL_PREFIX to
    # "/usr", which would silently prepend another /usr (systemd units would
    # then point at the wrong paths).  Pin the prefix to "/" so the relative
    # DESTINATIONs (usr/bin, usr/share/linux-apps, usr/lib/systemd/system, ...)
    # land where the maintainer scripts and unit files expect them.
    set(CPACK_PACKAGING_INSTALL_PREFIX "/")

    # Third-party shared libraries (glib, libxml2, fwupd, mbim, udev, ...)
    # are resolved from the system at run time via the declared package
    # dependencies below -- no private copies are bundled and the staged
    # binaries are not modified at packaging time (Debian UpstreamGuide:
    # "Do not include third-party code").  Direct NEEDED entries of the
    # prebuilt binaries (verified with ldd):
    #   rolling_config : glib, libxml2
    #   rolling_flash  : glib, libxml2, libfwupd
    #   rolling_helper : glib, libmbim-glib, libudev, libstdc++
    #   rolling_ma     : glib, libmbim-glib, libmm-glib
    # The SONAME-encoded package names (libxml2-16, libfwupd3,
    # libmbim-glib4, ...) match Ubuntu 26.04; re-check them against the
    # target release's package database when that changes.
    #
    # init-system-helpers provides deb-systemd-helper / deb-systemd-invoke,
    # which the maintainer scripts use instead of raw systemctl
    # (Policy 9.3.x, upstream review 2026-11).
    set(_rolling_deb_runtime_depends
        "init-system-helpers, libc6, modemmanager, libmm-glib0, libglib2.0-0, libxml2-16, libfwupd3, libmbim-glib4 (>= 1.18), libudev1, libstdc++6, libgcc-s1")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "${_rolling_deb_runtime_depends}")
    message(STATUS "Debian package depends: ${CPACK_DEBIAN_PACKAGE_DEPENDS}")
endmacro()

# --- Packaging: fixed logic for the binary-only tree -------------------------
#
# All services ship as prebuilt executables under application/<service>/, so
# there is no per-platform source tree to walk any more.
#
# Universal package (2.1.11): ONE package per project covering every OEM --
# the OEM-specific helper variants, ini sets and FCC unlock backends all
# ship inside it and are selected at runtime via the DMI vendor.
# OEM_BUILD=all (or unset): universal package, name without -<oem>, shipping
# every OEM section's assets and the union BUILD flags.  A specific
# OEM_BUILD: single-OEM package, name with -<oem>, shipping only that
# section's assets and BUILD flags.
if(PROJECT_BUILD STREQUAL "rw350r")
    set(ROLLING_PLATFORM "mtk")
else()
    set(ROLLING_PLATFORM "qualcomm")
endif()
if(OEM_BUILD STREQUAL "all" OR OEM_BUILD STREQUAL "")
    set(CPACK_PACKAGE_NAME "linux-apps-${ROLLING_PLATFORM}-${PROJECT_BUILD}")
else()
    set(CPACK_PACKAGE_NAME "linux-apps-${ROLLING_PLATFORM}-${PROJECT_BUILD}-${OEM_BUILD}")
endif()

# Migration from the 2.x per-OEM packages (linux-apps-<plat>-<proj>-<oem>):
# Breaks + Conflicts + Replaces let dpkg remove/replace any installed
# per-OEM package instead of failing on the shared file paths
# (/usr/bin/rolling_helper, /usr/bin/rolling_config, ...).  The list comes
# from the OEM sections of <project>_config (emitted by cmake_config).
# NOTE: cmake_config emits OEM_SECTIONS space-separated (shell-friendly for
# the packaging scripts); CMake lists are semicolon-separated, so convert
# and drop the empty item produced by the trailing space.
set(_rolling_oem_list "${OEM_SECTIONS}")
string(REPLACE " " ";" _rolling_oem_list "${_rolling_oem_list}")
list(REMOVE_ITEM _rolling_oem_list "")
set(_rolling_legacy_oem_packages "")
foreach(_oem ${_rolling_oem_list})
    if(_rolling_legacy_oem_packages STREQUAL "")
        set(_rolling_legacy_oem_packages
            "linux-apps-${ROLLING_PLATFORM}-${PROJECT_BUILD}-${_oem} (<< ${PROJECT_VERSION})")
    else()
        string(APPEND _rolling_legacy_oem_packages
            ", linux-apps-${ROLLING_PLATFORM}-${PROJECT_BUILD}-${_oem} (<< ${PROJECT_VERSION})")
    endif()
endforeach()

# Take over the files of the 2.x per-OEM packages (see comment above).
set(CPACK_DEBIAN_PACKAGE_BREAKS "${_rolling_legacy_oem_packages}")
set(CPACK_DEBIAN_PACKAGE_CONFLICTS "${_rolling_legacy_oem_packages}")
set(CPACK_DEBIAN_PACKAGE_REPLACES "${_rolling_legacy_oem_packages}")

# 使用install脚本创建的目录结构
set(deb_soft_dirs "${PROJECT_BINARY_DIR}/release/dpkg")

# NOTE: DESTINATION must be RELATIVE.  An absolute
# destination is used verbatim and is NOT relocated by
# DESTDIR, so "cmake --install" would write straight into
# the host /opt and /usr and debhelper would find
# debian/tmp empty.  A relative destination is resolved
# against CMAKE_INSTALL_PREFIX and relocated by DESTDIR,
# which is exactly what dh_auto_install needs.
# debian/rules passes -DCMAKE_INSTALL_PREFIX=/ and dh
# supplies DESTDIR=debian/tmp, giving debian/tmp/opt/...

# Permissions are taken from the staged tree: the
# upstream "install" helper normalises it to 0755 for
# executables/directories and 0644 for data files.
# USE_SOURCE_PERMISSIONS is required: without any
# permission keyword install(DIRECTORY) installs every
# file as 0644 and silently drops the executable bit
# from the service binaries (systemd then fails with
# 203/EXEC).  Do NOT set FILE_PERMISSIONS either --
# it would override the staged modes and make a cpack
# build differ from a dpkg-buildpackage build.

# 安装二进制与数据目录 (FHS / Policy 9.1.1: no /opt, upstream review 2026-11)
# Binaries -> /usr/bin, runtime data -> /usr/share/linux-apps (package-name
# directory per FHS, Canonical review 2026-11).  Nothing is staged under
# /etc: the ini files are runtime-seeded into /etc/rolling/config_file by
# seed_config.sh / rolling_seed_config.service (universal package, 2.1.11),
# so the maintainers own the /etc lifecycle, not dpkg conffiles.
                        install(DIRECTORY "${deb_soft_dirs}/usr/bin/"
                                DESTINATION "usr/bin"
                                USE_SOURCE_PERMISSIONS)

                        install(DIRECTORY "${deb_soft_dirs}/usr/share/linux-apps/"
                                DESTINATION "usr/share/linux-apps"
                                USE_SOURCE_PERMISSIONS)

                        # OEM helper variants (no bundled
                        # vendor libraries any more); the staging directory
                        # only exists when helper/MA components are built,
                        # so the copy is
                        # conditional at install time (a configure-time
                        # if(EXISTS) would always be false -- the staging tree
                        # is generated by the "install" helper at install
                        # time, after configure).
                        install(CODE "
                            if(EXISTS \"${deb_soft_dirs}/usr/lib/rolling\")
                                file(INSTALL \"${deb_soft_dirs}/usr/lib/rolling\"
                                     DESTINATION \"usr/lib\"
                                     USE_SOURCE_PERMISSIONS)
                            endif()
                        ")
                        
                        # 安装udev相关目录
                        # NOTE: install(DIRECTORY ...) only accepts permission
                        # tokens after FILE_PERMISSIONS / DIRECTORY_PERMISSIONS.
                        # A bare "OWNER_EXECUTE ..." is not a keyword there -- it
                        # is parsed as one more *source directory*, and the build
                        # dies with
                        #   file INSTALL cannot find ".../OWNER_EXECUTE"
                        # Permissions therefore come from the staged tree via
                        # USE_SOURCE_PERMISSIONS (the "install" helper
                        # normalises it to 0755/0644).
                        install(DIRECTORY "${deb_soft_dirs}/usr/lib/udev/rules.d/"
                                DESTINATION "usr/lib/udev/rules.d"
                                USE_SOURCE_PERMISSIONS)

                        # 安装systemd服务目录
                        # Policy 4.7.1 (10.1): must not install into /lib etc.
                        # (now symlinks to /usr) -- use usr/lib/systemd/system.
                        install(DIRECTORY "${deb_soft_dirs}/usr/lib/systemd/system/"
                                DESTINATION "usr/lib/systemd/system"
                                USE_SOURCE_PERMISSIONS)

                        # 安装dbus配置目录
                        install(DIRECTORY "${deb_soft_dirs}/usr/share/dbus-1/system.d/"
                                DESTINATION "usr/share/dbus-1/system.d"
                                USE_SOURCE_PERMISSIONS)

                        # Manual pages (Debian Policy 12.1): the "install"
                        # helper stages them pre-gzipped under
                        # usr/share/man/man{5,8}, so a plain directory copy
                        # with USE_SOURCE_PERMISSIONS (0644 after
                        # normalize_permissions) is enough.
                        install(DIRECTORY "${deb_soft_dirs}/usr/share/man/"
                                DESTINATION "usr/share/man"
                                USE_SOURCE_PERMISSIONS)

                        # NOTE: no copyright/doc installation here (Debian
                        # policy 12.5, upstream review 2026-11).  The copyright
                        # file must be installed by the downstream packaging
                        # (dh_installdocs) at
                        # /usr/share/doc/<binary-package-name>/copyright; a
                        # generic /usr/share/doc/linux-apps path is a policy
                        # violation and collision-prone.
                        install(CODE "MESSAGE(\"install success!\")")

                        set(CPACK_PACKAGE_VENDOR "rolling")

                        if (${BUILD_DEB} STREQUAL "yes")
                            set(CPACK_GENERATOR "DEB")
                            set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
                            set(CPACK_DEBIAN_PACKAGE_GROUP "rolling")
                            set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "WWAN Linux apps")
                            set(CPACK_DEBIAN_PACKAGE_SECTION "net")
                            set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
                            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
                            set(CPACK_DEBIAN_PACKAGE_MAINTAINER "rolling")
                            rolling_linux_configure_deb_package()
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
                            set(CPACK_RPM_PACKAGE_LICENSE "LGPL-2.1 AND GPL-2.0")
                            set(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE "${PROJECT_DEB_CONTROL_PREINST}")
                            set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${PROJECT_DEB_CONTROL_POSTINST}")
                            set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${PROJECT_DEB_CONTROL_PRERM}")
                            set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${PROJECT_DEB_CONTROL_POSTRM}")
                            set(CPACK_RPM_RELOCATION_PATHS "/usr/lib/systemd/system;/usr/bin;/usr/share/linux-apps")
                        else()
                            set(CPACK_GENERATOR "DEB")
                            set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
                            set(CPACK_DEBIAN_PACKAGE_GROUP "rolling")
                            set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "WWAN Linux apps")
                            set(CPACK_DEBIAN_PACKAGE_SECTION "net")
                            set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
                            set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
                            set(CPACK_DEBIAN_PACKAGE_MAINTAINER "rolling")
                            rolling_linux_configure_deb_package()
                            set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
                                "${PROJECT_DEB_CONTROL_PREINST};${PROJECT_DEB_CONTROL_POSTINST};${PROJECT_DEB_CONTROL_PRERM};${PROJECT_DEB_CONTROL_POSTRM}")
                        endif()

# set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/CPackConfig_qualcomm_rw135_lenovo.cmake")

include(CPack)




configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
  @ONLY
)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
