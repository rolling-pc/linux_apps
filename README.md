# Rolling Linux apps
This is a Rolling Linux apps set project for WWAN devices.<br>
  **Flash service:** firmware update, switch, recovery.<br>
  **Config service:** OEM configuration function.<br>
  **Helper service:** provides D-Bus API for Flash/Ma/Config service.<br>

# License
The rolling_flash, rolling_config, and rolling_helper binaries are LGPL 2.0.<br>

# Notice
  - Services must be used with fw_package. Before installing services, ensure that fw_package has been installed. Obtain the fw package from the corresponding OEM.<br>
  - fw_switch uses fastboot; you can install fastboot with `sudo apt-get install fastboot`<br>
  - This application runs on Ubuntu 24.04; Fedora is in testing; other Ubuntu versions and other OSes are unverified.

# Building on Ubuntu

## 1. Install

- sudo apt install cmake<br>
- sudo apt install build-essential<br>
- sudo apt install -y pkg-config<br>
- sudo apt install libglib2.0-dev<br>
- sudo apt install libxml2-dev<br>
- sudo apt install libudev-dev<br>
- sudo apt install libmbim-glib-dev<br>
- sudo apt install libdbus-1-dev<br>
- sudo apt install libmm-glib-dev<br>
- sudo apt install libxml2-dev<br>
- sudo apt install libfwupd-dev<br>

## 2. Build
1. cmake -S . -B build -DPROJECT_BUILD=xxx -DOEM_BUILD=yyy<br>
    Example (rw101 lenovo):<br>
    `cmake -S . -B build -DBUILD_DEB=yes -DPROJECT_BUILD=rw101 -DOEM_BUILD=lenovo -DBUILD_BY_LIB=1` <br>
    Example (rw350r lenovo):<br>
    `cmake -S . -B build -DBUILD_DEB=yes -DPROJECT_BUILD=rw350r -DOEM_BUILD=lenovo -DBUILD_BY_LIB=1` <br>
    Example (rw350r dell):<br>
    `cmake -S . -B build -DBUILD_DEB=yes -DPROJECT_BUILD=rw350r -DOEM_BUILD=dell -DBUILD_BY_LIB=1` <br>
    Or use script:<br>
    `./script/make_deb.sh deb rw350r lenovo`<br>
    `./script/make_deb.sh rpm rw350r lenovo`<br>
    `./script/make_deb.sh deb rw350r dell`<br>
    `./script/make_deb.sh rpm rw350r dell`<br>

### 2.1 Multi-distro deb (Docker, by-lib)

This repo only has prebuilt static libs. Build libs in **opensrc** Docker first, then package here.

```bash
# opensrc: build 26.04 libs (+ rolling_ma)
cd /path/to/dev_linux_app/.../linux-apps
./script/build_multi_distro.sh lib rw350r dell resolute
cp -a binary_deb/common_lib-ubuntu26.04/* \
  /path/to/upstream_github/linux_apps/binary_deb/common_lib-ubuntu26.04/

# upstream: package 26.04 deb inside Docker (always by-lib)
cd /path/to/upstream_github/linux_apps
./script/build_multi_distro.sh deb rw350r dell resolute
# skip smoke: add --no-smoke
```

Library Ubuntu version must match the deb target (22.04 libs for jammy, 26.04 for resolute). Do not link 26.04 libs on a 22.04 host.

## 3. If using systemd
- Reload config:<br>
  sudo systemctl daemon-reload
- Enable service:<br>
  sudo systemctl enable rolling_xxx.service<br>
  **Examples:**<br>
  sudo systemctl enable rolling_helper.service<br>
  sudo systemctl enable rolling_flash.service<br>
  sudo systemctl enable rolling_config.service<br>
  **Note:** This step must be done so that systemd can find and start the service.<br>
- Start service:<br>
  sudo systemctl start rolling_xxx.service<br>
- Get status:<br>
  sudo systemctl status rolling_xxx.service<br>
- Stop service:<br>
  sudo systemctl stop rolling_xxx.service<br>

## 4. Items not copied when installing (setup.sh)

When you build with `setup.sh`, files under `service/` are copied to the system directories. The following items are **not** copied:

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
