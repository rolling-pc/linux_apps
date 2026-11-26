# Rolling Linux apps
This is a Rolling Linux apps set project for WWAN devices.<br>
  **Flash service:** firmware update, switch, recovery.<br>
  **Config service:** OEM configuration function.<br>
  **Helper service:** provides D-Bus API for Flash/Ma/Config service.<br>
  **Ma service:** provides the rolling_ma binary (ModemManager FCC unlock helper).<br>
  Note: all four services are shipped as prebuilt binaries only; the service
  implementation sources are not published in this repository.
  Note: since 2.1.11 the package is UNIVERSAL -- one package per project
  (e.g. `linux-apps-mtk-rw350r`) covers every OEM.  All OEM-specific assets
  ship inside it and are selected at runtime via the machine vendor (DMI):
  rolling_helper variants live under
  `/usr/lib/rolling/rolling_helper_service/helper_set/<project>_<oem>/` and
  `/usr/bin/rolling_helper` is a dispatcher that execs the matching one; the
  ini files ship as per-OEM templates under
  `/usr/share/linux-apps/{config,flash}_set/<project>_<oem>/` and are seeded
  into `/etc/rolling/config_file` by `seed_config.sh` (postinst + a
  first-boot oneshot unit); the FCC unlock scripts dispatch on the DMI
  machine vendor to run rolling_ma on Dell machines.

# License
Licensing is described in one place, the top-level LICENSE file:

  The whole project -- the published files (*.h, CMakeLists.txt, script/,
  cmake_config/, rw350r_config/) AND the four prebuilt service executables
  (rolling_config, rolling_flash, rolling_helper, rolling_ma) -- is
  licensed under the GNU Lesser General Public License, version 2.1 or
  later (LGPL-2.1-or-later); see LICENSE "SECTION A" (same text in
  COPYING).  Exception: application/include/common/rolling_list.h is
  GPL-2 (derived from the Linux kernel); third-party code incorporated
  into the binaries (dmidecode -> GPL-2, iniparser / safestringlib ->
  MIT) keeps its own license, as recorded in each service directory's
  LICENSE copy (application/rolling_config_service/LICENSE, ...).

  Each service directory ships a LICENSE copy listing the size and
  SHA-256 digest of the binaries it covers, so a redistributed copy can
  be verified against the exact artefacts.

  Debian note: the four service executables ship in binary form only --
  the service implementation sources (*.c / *.cc) are not published in
  this repository, so the binaries cannot be rebuilt from this tree.
  Even though they carry the LGPL-2.1+ license, the missing preferred
  form for modification means the package does not satisfy the Debian
  Free Software Guidelines and therefore cannot enter Debian main; it
  is distributed through the OEM channels instead (non-free would be
  the only Debian-adjacent option, which we intentionally do not
  target).
  The downstream packaging ships debian/copyright at the standard
  per-package location /usr/share/doc/<binary-package-name>/copyright;
  we intentionally no longer install a generic
  /usr/share/doc/linux-apps/ tree from the upstream build.<br>

# Notice
  - Services must be used with fw_package. Before installing services, ensure that fw_package has been installed. Obtain the fw package from the corresponding OEM.<br>
  - fw_switch uses fastboot; you can install fastboot with `sudo apt-get install fastboot`<br>
  - This application runs on **Ubuntu 26.04 or later**. <br>

# Building on Ubuntu

This is a binary-only tree: no compilation is needed, the four service
executables ship prebuilt in application/rolling_*_service/.

## 1. Install

- sudo apt install cmake<br>

## 2. Build
1. cmake -S . -B build -DPROJECT_BUILD=xxx -DOEM_BUILD=yyy<br>
    Example (rw350r lenovo):<br>
    `cmake -S . -B build -DBUILD_DEB=yes -DPROJECT_BUILD=rw350r -DOEM_BUILD=lenovo -DBUILD_BY_LIB=1` <br>
    Example (rw350r dell, Ubuntu 26.04):<br>
    `cmake -S . -B build -DBUILD_DEB=yes -DPROJECT_BUILD=rw350r -DOEM_BUILD=dell -DBUILD_BY_LIB=1 -DDISTRO_TAG=ubuntu26.04` <br>
2. cmake --build build<br>
3. cd build && cpack -- produces the deb in build/.<br>

OEM_BUILD semantics:<br>
- `-DOEM_BUILD=all`: ONE universal package (name `linux-apps-mtk-rw350r`)
  containing the assets of every OEM section found in `<project>_config`
  (currently lenovo and dell for rw350r), with the union of their BUILD
  flags; variants are selected at runtime via the DMI vendor.  Adding a new
  OEM requires no change, just its config section plus the
  helper_set/config_set/flash_set directories.<br>
- `-DOEM_BUILD=dell` (or another single OEM): a per-OEM package (name
  `linux-apps-mtk-rw350r-dell`) shipping only that section's assets and
  BUILD flags.<br>
- Leaving `OEM_BUILD` unset behaves like `all`.<br>

Note: every OEM section of `<project>_config` must have its
`helper_set/<project>_<oem>/rolling_helper` variant and its
`config_set`/`flash_set` ini directories -- the build fails early otherwise,
because the universal package would leave that OEM's machines without a
working helper or config.

Note: when running the manual cmake/cpack route several times, remove the
build/ directory between runs (`rm -rf build`). The install script greps
build/*_config.cmake with a glob, so a stale config file left over from the
previous run would be picked up together with the new one and corrupt the
package.

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

- **Binaries:** `/usr/bin/` (rolling_config, rolling_flash, rolling_ma; rolling_helper is a generated DMI-dispatch wrapper)
- **OEM helper variants:** `/usr/lib/rolling/rolling_helper_service/helper_set/<project>_<oem>/rolling_helper` — every OEM's binary ships; `/usr/bin/rolling_helper` execs the one matching the machine vendor (DMI). An unknown vendor fails loudly instead of running a wrong-OEM helper.
- **Third-party libraries:** resolved from the system at run time via the declared package dependencies (libglib2.0-0, libxml2-16, libfwupd3, libmbim-glib4, libudev1, libmm-glib0, libstdc++6, ...); no private copies are bundled and the binaries are not modified at packaging time.
- **Configuration templates:** `/usr/share/linux-apps/config_set/<project>_<oem>/` and `/usr/share/linux-apps/flash_set/<project>_<oem>/` — every OEM's ini files ship as templates; the matching set is seeded into `/etc/rolling/config_file/...` by `/usr/share/linux-apps/seed_config.sh` (called by the postinst and by the `rolling_seed_config.service` first-boot oneshot). Seeded files are NOT dpkg conffiles: seeding never overwrites an existing file, so operator edits survive upgrades; `purge` removes them with `/etc/rolling`.
- **Runtime data (fcc-unlock scripts, fwupd quirk):** `/usr/share/linux-apps/` (package-name directory per FHS; Canonical review 2026-11)
- **udev rules (usr/lib/udev/rules.d/):** installed by the CMake build; ModemManager/fwupd read them from there.
- **systemd units:** `/usr/lib/systemd/system/rolling_*.service`.  The `LD_LIBRARY_PATH=...:/usr/local/lib` runtime variable used to live in a rolling_*.d/env.conf drop-in under /usr (review 2026-11 FHS compliance issue); it is now inlined as a plain `Environment=` directive in each unit, so no drop-in directory is staged.
- **D-Bus policy:** `/usr/share/dbus-1/system.d/com.rolling.helper.conf`
- **Manual pages:** `/usr/share/man/man8/` (rolling_config, rolling_flash, rolling_helper, rolling_ma) and `/usr/share/man/man5/` (rwwwanConfig.ini, FwUpdate.ini) — view with e.g. `man 8 rolling_config`, `man 5 rwwwanConfig.ini`

Note: the ini files are seeded into `/etc/rolling/config_file/` at runtime (postinst best-effort + `rolling_seed_config.service` oneshot while the config is missing; never overwritten once present); the runtime state directories `/var/lib/rolling/run` and `/var/lib/rolling/rolling_fw_pkg` are created by the postinst (their paths are compiled into the binaries). When upgrading from a pre-2.1.12 revision, the preinst carries over operator-edited ini files from the legacy `/etc/opt/rolling` layout. When upgrading from a 2.x per-OEM package (`linux-apps-<platform>-<project>-<oem>`), dpkg replaces it automatically via the declared Breaks/Conflicts/Replaces.

## 5. fccunlock
- location path:<br>
  rolling_ma : /usr/bin/rolling_ma<br>
  shipped scripts : /usr/share/linux-apps/rolling_ma_service/fcc-unlock.d<br>
  14c3:4d75 : /usr/lib/x86_64-linux-gnu/ModemManager/fcc-unlock.d (symlink to the shipped script)
- working:<br>
  rolling_ma while be call by Modemmanager, when Modemmanager detect the modem is locked.<br>
  The shipped scripts dispatch on the machine vendor (DMI): Dell machines run rolling_ma; Lenovo machines are unlocked by an external vendor tool.

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
