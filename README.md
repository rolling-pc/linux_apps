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
    Example: if xxx is rw101 and yyy is lenovo, you can use:<br>
    `cmake -S . -B build -DBUILD_DEB=yes -DPROJECT_BUILD=rw101 -DOEM_BUILD=lenovo -DBUILD_BY_LIB=1` <br>
### 2.1 Build lib
  1. To build helper lib:<br>
    Build all helper libs: `cmake -S . -B build -DBUILD_LIB=y`<br>
    Or build one lib: `cmake -S . -B build -DBUILD_LIB=y -DPROJECT_BUILD=rw101 -DOEM_BUILD=lenovo`<br>
  2. cmake --build build<br>
  3. cd build && make build_lib<br>
  4. You will find `common_lib` in the `build` directory.

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

# Release history
- version 1.0.0<br>
  First version, uploaded to GitHub.<br>
