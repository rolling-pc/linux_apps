#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  ./setup.sh <deb|rpm> <project> <oem> [--no-install]

What it does:
  - Builds services using prebuilt libraries (binary_deb/common_lib), without packaging.
  - Prepares the build environment (by default installs deps via apt if needed).
  - Outputs "ready to package" files under the service/ directory (no .deb/.rpm is produced).

Arguments:
  1) deb|rpm    layout type (deb or rpm; same layout for this script)
  2) project    e.g. rw101
  3) oem        e.g. lenovo

Optional:
  --no-install  do not install dependencies; only build

Examples:
  ./setup.sh deb rw101 lenovo
  ./setup.sh deb rw101 lenovo --no-install
EOF
}

die() { echo "ERROR: $*" >&2; exit 1; }
log() { echo "[setup] $*"; }

PKG_TYPE="${1:-}"
PROJECT_BUILD="${2:-}"
OEM_BUILD="${3:-}"
NO_INSTALL="no"
if [[ "${4:-}" == "--no-install" ]]; then
  NO_INSTALL="yes"
elif [[ -n "${4:-}" ]]; then
  usage
  die "Unknown option: ${4}"
fi

[[ -n "${PKG_TYPE}" && -n "${PROJECT_BUILD}" && -n "${OEM_BUILD}" ]] || { usage; exit 1; }
[[ "${PKG_TYPE}" == "deb" || "${PKG_TYPE}" == "rpm" ]] || die "Argument 1 must be 'deb' or 'rpm'"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Support running from repo root (opensrc) or from linux-apps
if [[ -f "${SCRIPT_DIR}/script/make_deb.sh" ]]; then
  LINUX_APPS_DIR="${SCRIPT_DIR}"
else
  LINUX_APPS_DIR="${SCRIPT_DIR}/linux-apps"
fi
[[ -d "${LINUX_APPS_DIR}" ]] || die "Directory not found: ${LINUX_APPS_DIR}"
[[ -f "${LINUX_APPS_DIR}/install" ]] || die "install script not found under ${LINUX_APPS_DIR}"

need_cmds=(cmake make gcc pkg-config)
missing_cmds=()
for c in "${need_cmds[@]}"; do
  command -v "${c}" >/dev/null 2>&1 || missing_cmds+=("${c}")
done

if [[ "${NO_INSTALL}" == "no" ]]; then
  if [[ "${#missing_cmds[@]}" -gt 0 ]]; then
    log "Missing commands: ${missing_cmds[*]}"
  fi
  if command -v apt-get >/dev/null 2>&1; then
    if [[ "${EUID}" -ne 0 ]]; then
      die "Root required to install dependencies. Use sudo or --no-install."
    fi
    log "Installing dependencies..."
    export DEBIAN_FRONTEND=noninteractive
    apt-get update -y
    apt-get install -y \
      cmake build-essential pkg-config \
      libglib2.0-dev libxml2-dev libudev-dev \
      libmbim-glib-dev libdbus-1-dev libmm-glib-dev libfwupd-dev \
      fastboot
    log "Dependencies installed."
  else
    log "apt-get not found; skipping dependency installation (use --no-install)."
  fi
else
  log "--no-install: skipping dependency installation."
fi

# Build with library files (build_by_lib=1), no packaging
BUILD_BY_LIB=1
log "Building services (library mode, no package): ${PKG_TYPE} ${PROJECT_BUILD} ${OEM_BUILD}"
cd "${LINUX_APPS_DIR}"

# Keep common_lib when present
if [[ -d "binary_deb" && -d "binary_deb/common_lib" ]]; then
  log "Keeping binary_deb/common_lib for library build."
  find binary_deb -mindepth 1 -maxdepth 1 -not -name "common_lib" -exec rm -rf {} \; 2>/dev/null || true
else
  rm -rf binary_deb
  mkdir -p binary_deb
fi

[[ -d "build" ]] && rm -rf build
cmake -S . -B build \
  -DBUILD_DEB=yes \
  -DPROJECT_BUILD="${PROJECT_BUILD}" \
  -DOEM_BUILD="${OEM_BUILD}" \
  -DBUILD_BY_LIB="${BUILD_BY_LIB}"
cmake --build build
# Do not run cpack

# Run install script to populate build/release/dpkg (ready-to-package layout)
log "Preparing install layout (install script)..."
bash ./install

# Copy ready-to-package files into service/
SERVICE_DIR="${LINUX_APPS_DIR}/service"
DPKG_DIR="${LINUX_APPS_DIR}/build/release/dpkg"
if [[ ! -d "${DPKG_DIR}" ]]; then
  die "Expected directory not found: ${DPKG_DIR}. Install script may have failed."
fi
rm -rf "${SERVICE_DIR}"
mkdir -p "${SERVICE_DIR}"
cp -a "${DPKG_DIR}"/* "${SERVICE_DIR}/"
log "Done. Service files (ready to package) are in: ${SERVICE_DIR}/"

# Copy service/ to system (excluding usr/lib/udev and rolling_*.d/env.conf)
# Requires root; uses sudo when not running as root.
SUDO=""
[[ "${EUID}" -ne 0 ]] && SUDO="sudo"
log "Copying service files to system (excluding usr/lib/udev and rolling_*.d/env.conf)..."
if [[ -d "${SERVICE_DIR}/opt" ]]; then
  ${SUDO} mkdir -p /opt
  ${SUDO} cp -a "${SERVICE_DIR}/opt"/* /opt/
fi
if [[ -d "${SERVICE_DIR}/lib" ]]; then
  ${SUDO} mkdir -p /lib
  ${SUDO} cp -a "${SERVICE_DIR}/lib"/* /lib/
  for drop in rolling_config.d/env.conf rolling_flash.d/env.conf rolling_helper.d/env.conf; do
    ${SUDO} rm -f "/lib/systemd/system/${drop}"
  done
fi
if [[ -d "${SERVICE_DIR}/usr" ]]; then
  ${SUDO} mkdir -p /usr
  # Copy usr but skip usr/lib/udev entirely
  if [[ -d "${SERVICE_DIR}/usr/share" ]]; then
    ${SUDO} mkdir -p /usr/share
    ${SUDO} cp -a "${SERVICE_DIR}/usr/share"/* /usr/share/
  fi
  if [[ -d "${SERVICE_DIR}/usr/lib" ]]; then
    ${SUDO} mkdir -p /usr/lib
    for item in "${SERVICE_DIR}/usr/lib"/*; do
      [[ -e "${item}" ]] || continue
      [[ "$(basename "${item}")" == "udev" ]] && continue
      ${SUDO} cp -a "${item}" /usr/lib/
    done
  fi
fi
log "Service files copied to system (usr/lib/udev and rolling_*.d/env.conf skipped)."
