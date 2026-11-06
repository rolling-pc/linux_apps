# Rolling Linux apps
This is a Rolling Linux apps set project for WWAN devices.<br>
  **Flash service:** firmware update, switch, recovery.<br>
  **Config service:** OEM configuration function.<br>
  **Helper service:** provides D-Bus API for Flash/Ma/Config service.<br>
  **Ma service:** provides the rolling_ma binary (ModemManager FCC unlock helper).<br>
  Note: all four services are shipped as prebuilt binaries only; the service
  implementation sources are not published in this repository.

# License
Licensing is described in one place, the top-level LICENSE file:

  1. SOURCE CODE -- this repository no longer publishes the service
     implementation sources (*.c / *.cc).  The published files (*.h,
     CMakeLists.txt, script/, cmake_config/, rw350r_config/) are LGPL-2.1
     (LICENSE "SECTION A"; same text in COPYING).  Exception:
     application/include/common/rolling_list.h is GPL-2 (derived from the
     Linux kernel).<br>

  2. PREBUILT BINARIES -- the four service executables
     (rolling_config, rolling_flash, rolling_helper, rolling_ma) are
     compiled from internal sources that are not published in this
     repository.  Each is redistributed under its own Custom Binary
     Redistribution License (LICENSE "SECTION B"), shipped in its service
     directory with the size and SHA-256 of the binary it covers
     (application/rolling_config_service/LICENSE,
     application/rolling_flash_service/LICENSE,
     application/rolling_helper_service/LICENSE,
     application/rolling_ma_service/LICENSE).  The deb package aggregates
     all four licenses in /usr/share/doc/linux-apps/copyright, together
     with the MIT notices of the statically incorporated third-party code
     (iniparser in rolling_config, safestringlib in rolling_flash).<br>

# Notice
  - Services must be used with fw_package. Before installing services, ensure that fw_package has been installed. Obtain the fw package from the corresponding OEM.<br>
  - fw_switch uses fastboot; you can install fastboot with `sudo apt-get install fastboot`<br>
  - This application is packaged for Ubuntu 24.04 by default; the deb release tag can be set explicitly with `-DDISTRO_TAG=ubuntu26.04` (or ubuntu22.04). Fedora and other OSes are unverified.

# Building on Ubuntu

This is a binary-only tree: no compilation is needed, the four service
executables ship prebuilt in application/rolling_*_service/.

## 1. Install

- sudo apt install cmake<br>

## 2. Build
1. cmake -S . -B build -DPROJECT_BUILD=xxx -DOEM_BUILD=yyy<br>
    Example (rw101 lenovo):<br>
    `cmake -S . -B build -DBUILD_DEB=yes -DPROJECT_BUILD=rw101 -DOEM_BUILD=lenovo -DBUILD_BY_LIB=1` <br>
    Example (rw350r lenovo):<br>
    `cmake -S . -B build -DBUILD_DEB=yes -DPROJECT_BUILD=rw350r -DOEM_BUILD=lenovo -DBUILD_BY_LIB=1` <br>
    Example (rw350r dell, Ubuntu 26.04):<br>
    `cmake -S . -B build -DBUILD_DEB=yes -DPROJECT_BUILD=rw350r -DOEM_BUILD=dell -DBUILD_BY_LIB=1 -DDISTRO_TAG=ubuntu26.04` <br>
2. cd build<br>
3. cpack -- produces the deb in build/.<br>
Or use script (the finished .deb is also moved to binary_deb/):<br>
`./script/make_deb.sh deb rw350r lenovo`<br>
`./script/make_deb.sh rpm rw350r lenovo`<br>
`./script/make_deb.sh deb rw350r dell`<br>
`./script/make_deb.sh rpm rw350r dell`<br>

## 3. If using systemd
- Reload config:<br>
  sudo systemctl daemon-reload
- Enable service:<br>
  sudo systemctl enable rolling_xxx.service<br>
  **Examples:**<br>
  sudo systemctl enable rolling_helper.service<br>
  sudo systemctl enable rolling_flash.service<br>
  sudo systemctl enable rolling_config.service<br>
  sudo systemctl enable rolling_ma.service<br>
  **Note:** This step must be done so that systemd can find and start the service.<br>
- Start service:<br>
  sudo systemctl start rolling_xxx.service<br>
- Get status:<br>
  sudo systemctl status rolling_xxx.service<br>
- Stop service:<br>
  sudo systemctl stop rolling_xxx.service<br>

## 4. Items not copied when installing (deb package)

When you install the deb package (built via the `install` script or debian packaging), files are copied to the system directories. The following items are **not** copied:

- **udev rules (usr/lib/udev/):** They are not copied because ModemManager already includes the content of these rules; no need to install them again.
- **env.conf (lib/systemd/system/rolling_*.d/env.conf):** These files only set environment variable paths (e.g. LD_LIBRARY_PATH). The current build does not use libraries from that path, so they are not needed.

## 5. fccunlock
- location path:<br>
  rolling_ma : /opt/rolling/rolling_ma_service/.
  14c3:4d75 : /usr/lib/x86_64-linux-gnu/ModemManager/fcc-unlock.d
- working:<br>
  rolling_ma while be call by Modemmanager, when Modemmanager detect the modem is locked.

# Release history
- version 1.0.0<br>
  First version, uploaded to GitHub.<br>
