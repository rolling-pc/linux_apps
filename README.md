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
     application/rolling_ma_service/LICENSE).  The downstream packaging
     ships debian/copyright at the standard per-package location
     /usr/share/doc/<binary-package-name>/copyright; we intentionally no
     longer install a generic /usr/share/doc/linux-apps/ tree from the
     upstream build (Debian policy 12.5, review 2026-11).<br>

# Notice
  - Services must be used with fw_package. Before installing services, ensure that fw_package has been installed. Obtain the fw package from the corresponding OEM.<br>
  - fw_switch uses fastboot; you can install fastboot with `sudo apt-get install fastboot`<br>
  - This application runs on **Ubuntu 26.04 or later**. <br>

# Building on Ubuntu

This is a binary-only tree: no compilation is needed, the four service
executables ship prebuilt in application/rolling_*_service/.

## 1. Install

- sudo apt install cmake patchelf<br>
  (`patchelf` is required by the packaging step: it appends the
  `/usr/lib/rolling/vendor/lib` RUNPATH to the prebuilt binaries so they
  resolve the bundled third-party libraries; without it the deb cannot be
  produced.)

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

## 4. Installed system files (deb package)

When you install the deb package (built via the `install` script or debian packaging), files are copied to the system directories:

- **Binaries:** `/usr/bin/` (rolling_config, rolling_flash, rolling_helper, rolling_ma)
- **Configuration (dpkg conffiles, Policy 10.7):** `/etc/rolling/config_file/rolling_config_service/` and `/etc/rolling/config_file/rolling_flash_service/` — the ini files are owned by dpkg, so upgrades get the standard conffile merge/prompt handling and `dpkg -V` can verify them
- **Runtime data (fcc-unlock scripts, fwupd quirk):** `/usr/share/linux-apps/` (package-name directory per FHS; Canonical review 2026-11)
- **udev rules (usr/lib/udev/rules.d/):** installed by the CMake build; ModemManager/fwupd read them from there.
- **systemd units:** `/usr/lib/systemd/system/rolling_*.service`.  The `LD_LIBRARY_PATH=...:/usr/local/lib` runtime variable used to live in a rolling_*.d/env.conf drop-in under /usr (review 2026-11 FHS compliance issue); it is now inlined as a plain `Environment=` directive in each unit, so no drop-in directory is staged.
- **D-Bus policy:** `/usr/share/dbus-1/system.d/com.rolling.helper.conf`

Note: the ini conffiles land directly in `/etc/rolling/config_file/` via dpkg (no postinst seeding); the runtime state directories `/var/lib/rolling/run` and `/var/lib/rolling/rolling_fw_pkg` are created by the postinst (their paths are compiled into the binaries). When upgrading from a pre-2.1.12 revision, the preinst carries over operator-edited ini files from the legacy `/etc/opt/rolling` layout.

## 5. fccunlock
- location path:<br>
  rolling_ma : /usr/bin/rolling_ma<br>
  shipped scripts : /usr/share/linux-apps/rolling_ma_service/fcc-unlock.d<br>
  14c3:4d75 : /usr/lib/x86_64-linux-gnu/ModemManager/fcc-unlock.d (symlink to the shipped script)
- working:<br>
  rolling_ma while be call by Modemmanager, when Modemmanager detect the modem is locked.

# Release history
- version 1.0.0<br>
  First version, uploaded to GitHub.<br>
- version 1.0.2<br>
  Fixed several known issues.<br>
- version 1.0.3<br>
  Fixed several known issues.<br>
- version 1.0.4<br>
  Fixed several known issues.<br>
- version 1.0.5<br>
  Comply with Debian policy: each service ships its own binary redistribution
  license.<br>
