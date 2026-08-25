#!/bin/bash
# Build linux-apps deb inside Ubuntu version-specific Docker images (by-lib only).
#
# upstream_github 仅有预编译静态库 + 薄封装源码，不能在容器内全量编 lib。
# 静态库请先在 opensrc 用同版本 Docker 编好，再同步到 binary_deb/common_lib-<tag>/。
#
# Usage:
#   ./script/build_multi_distro.sh deb <project> <oem> <distro> [--no-smoke]
#
# Examples:
#   # 1) opensrc 编 26.04 库
#   cd /path/to/dev_linux_app/.../linux-apps
#   ./script/build_multi_distro.sh lib rw350r dell resolute
#   cp -a binary_deb/common_lib-ubuntu26.04/* \
#     /path/to/upstream_github/linux_apps/binary_deb/common_lib-ubuntu26.04/
#
#   # 2) upstream 打 26.04 deb
#   ./script/build_multi_distro.sh deb rw350r dell resolute
#
# Distro aliases:
#   jammy     -> ubuntu:22.04
#   noble     -> ubuntu:24.04
#   resolute  -> ubuntu:26.04

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
DOCKER_DIR="${PROJECT_ROOT}/docker"

PKG_TYPE="${1:-}"
PROJECT="${2:-}"
OEM="${3:-}"
DISTRO="${4:-}"
# upstream 只能 by-lib；保留 --by-lib 兼容 opensrc 命令习惯
BUILD_BY_LIB=1
RUN_SMOKE=1

usage() {
    cat <<EOF
Usage:
  $0 deb <project> <oem> <distro> [--no-smoke]

Distro values:
  jammy     Ubuntu 22.04  -> common_lib-ubuntu22.04/
  noble     Ubuntu 24.04  -> common_lib-ubuntu24.04/
  resolute  Ubuntu 26.04  -> common_lib-ubuntu26.04/

upstream_github workflow:
  1. In opensrc: ./script/build_multi_distro.sh lib rw350r dell resolute
  2. cp -a opensrc/binary_deb/common_lib-ubuntu26.04/* \\
         upstream/binary_deb/common_lib-ubuntu26.04/
  3. In upstream: $0 deb rw350r dell resolute

Note: 'lib' is not supported here (no full sources). Build libs in opensrc.
EOF
}

resolve_distro() {
    case "$1" in
        jammy)
            DOCKERFILE="Dockerfile.jammy"
            BASE_IMAGE="ubuntu:22.04"
            DISTRO_TAG="ubuntu22.04"
            ;;
        noble)
            DOCKERFILE="Dockerfile.noble"
            BASE_IMAGE="ubuntu:24.04"
            DISTRO_TAG="ubuntu24.04"
            ;;
        resolute|ubuntu26.04|26.04)
            DOCKERFILE="Dockerfile.resolute"
            BASE_IMAGE="ubuntu:26.04"
            DISTRO_TAG="ubuntu26.04"
            ;;
        *)
            echo "Unsupported distro: $1" >&2
            usage
            exit 1
            ;;
    esac
    LIB_OUTPUT_DIR="${PROJECT_ROOT}/binary_deb/common_lib-${DISTRO_TAG}"
}

for arg in "$@"; do
    case "$arg" in
        --by-lib) BUILD_BY_LIB=1 ;;
        --no-smoke) RUN_SMOKE=0 ;;
    esac
done

if [[ -z "$PKG_TYPE" ]] || [[ -z "$PROJECT" ]] || [[ -z "$OEM" ]] || [[ -z "$DISTRO" ]]; then
    usage
    exit 1
fi

if [[ "$PKG_TYPE" == "lib" ]]; then
    echo "ERROR: upstream_github cannot build static libraries (sources stripped)." >&2
    echo "Build libs in opensrc, then sync:" >&2
    echo "  cd /path/to/dev_linux_app/.../linux-apps" >&2
    echo "  ./script/build_multi_distro.sh lib ${PROJECT} ${OEM} ${DISTRO}" >&2
    echo "  mkdir -p ${PROJECT_ROOT}/binary_deb/common_lib-\${DISTRO_TAG}" >&2
    echo "  cp -a binary_deb/common_lib-\${DISTRO_TAG}/* \\" >&2
    echo "    ${PROJECT_ROOT}/binary_deb/common_lib-\${DISTRO_TAG}/" >&2
    exit 1
fi

if [[ "$PKG_TYPE" != "deb" ]]; then
    echo "Unknown command: $PKG_TYPE (expected deb)" >&2
    usage
    exit 1
fi

resolve_distro "$DISTRO"

IMAGE_NAME="rolling-linux-apps-build:${DISTRO_TAG}"
SMOKE_IMAGE_NAME="rolling-linux-apps-smoke:${DISTRO_TAG}"

echo "==> Building image ${IMAGE_NAME} from ${DOCKERFILE}"
docker build -f "${DOCKER_DIR}/${DOCKERFILE}" -t "${IMAGE_NAME}" "${DOCKER_DIR}"

prepare_common_lib_for_by_lib() {
    mkdir -p "${PROJECT_ROOT}/binary_deb"

    # Prefer versioned dir; fall back to plain common_lib if it already has artifacts.
    if [[ ! -d "${LIB_OUTPUT_DIR}" ]]; then
        if [[ -d "${PROJECT_ROOT}/binary_deb/common_lib" ]]; then
            echo "==> ${LIB_OUTPUT_DIR} missing; seeding from binary_deb/common_lib"
            cp -a "${PROJECT_ROOT}/binary_deb/common_lib" "${LIB_OUTPUT_DIR}"
        else
            echo "ERROR: ${LIB_OUTPUT_DIR} not found." >&2
            echo "Sync opensrc libs first, e.g.:" >&2
            echo "  cp -a <opensrc>/binary_deb/common_lib-${DISTRO_TAG}/* ${LIB_OUTPUT_DIR}/" >&2
            exit 1
        fi
    fi

    if [[ "${PROJECT}" == "rw350r" && "${OEM}" == "dell" ]] && [[ ! -f "${LIB_OUTPUT_DIR}/rolling_ma" ]]; then
        echo "ERROR: ${LIB_OUTPUT_DIR}/rolling_ma not found." >&2
        echo "rolling_ma must be built in opensrc Docker (${DISTRO_TAG}) and synced here." >&2
        exit 1
    fi

    echo "==> Will use prebuilt libs from ${LIB_OUTPUT_DIR}"
    ls -la "${LIB_OUTPUT_DIR}"
}

verify_rolling_ma_for_rw350r_dell() {
    if [[ "${PROJECT}" != "rw350r" || "${OEM}" != "dell" ]]; then
        return 0
    fi

    local ma_path="$1"
    if [[ ! -f "${ma_path}" ]]; then
        echo "ERROR: rolling_ma missing at ${ma_path}" >&2
        exit 1
    fi

    echo "==> Verified rolling_ma: ${ma_path} ($(stat -c%s "${ma_path}") bytes)"
}

build_deb_in_container() {
    prepare_common_lib_for_by_lib

    echo "==> Building deb in container (${DISTRO_TAG}, by-lib)"
    docker run --rm \
        -e HOST_UID="$(id -u)" \
        -e HOST_GID="$(id -g)" \
        -e DISTRO_TAG="${DISTRO_TAG}" \
        -v "${PROJECT_ROOT}:/workspace" \
        -w /workspace \
        "${IMAGE_NAME}" \
        bash -lc "
            set -euo pipefail
            rm -rf /workspace/binary_deb/common_lib
            cp -a /workspace/binary_deb/common_lib-${DISTRO_TAG} /workspace/binary_deb/common_lib
            if [ -d build ]; then rm -rf build; fi
            if [ -d binary_deb ]; then
                find binary_deb -mindepth 1 -maxdepth 1 \
                    -not -name common_lib \
                    -not -name 'common_lib-*' \
                    -exec rm -rf {} + 2>/dev/null || true
            else
                mkdir -p binary_deb
            fi
            cmake -S . -B build \
                -DBUILD_DEB=yes \
                -DPROJECT_BUILD=${PROJECT} \
                -DOEM_BUILD=${OEM} \
                -DBUILD_BY_LIB=1 \
                -DDISTRO_TAG=${DISTRO_TAG}
            cmake --build build
            # Prefer prebuilt rolling_ma from common_lib; keep versioned copy in sync.
            if [ -f /workspace/binary_deb/common_lib/rolling_ma ]; then
                mkdir -p /workspace/binary_deb/common_lib-${DISTRO_TAG}
                cp -f /workspace/binary_deb/common_lib/rolling_ma \
                    /workspace/binary_deb/common_lib-${DISTRO_TAG}/rolling_ma
            fi
            cd build
            cpack
            mv *.deb /workspace/binary_deb/
            chown \"\${HOST_UID}:\${HOST_GID}\" /workspace/binary_deb/*.deb 2>/dev/null || true
            if [ -f /workspace/binary_deb/common_lib-${DISTRO_TAG}/rolling_ma ]; then
                chown \"\${HOST_UID}:\${HOST_GID}\" /workspace/binary_deb/common_lib-${DISTRO_TAG}/rolling_ma 2>/dev/null || true
            fi
        "

    verify_rolling_ma_for_rw350r_dell "${LIB_OUTPUT_DIR}/rolling_ma"
}

run_smoke_test() {
    local deb_file="$1"
    local xml_pkg="libxml2"
    local fwupd_pkg="libfwupd2"

    if [[ "${DISTRO_TAG}" == "ubuntu26.04" ]]; then
        xml_pkg="libxml2-16"
        fwupd_pkg="libfwupd3"
    fi

    echo "==> Running smoke test in clean ${BASE_IMAGE} container"
    docker build -f "${DOCKER_DIR}/${DOCKERFILE}" -t "${SMOKE_IMAGE_NAME}" "${DOCKER_DIR}"

    docker run --rm \
        -v "${deb_file}:/tmp/package.deb:ro" \
        "${SMOKE_IMAGE_NAME}" \
        bash -lc "
            set -euo pipefail
            apt-get update -qq

            echo '==> Checking deb Depends metadata'
            dpkg-deb -I /tmp/package.deb | grep -E '^ Depends:' | grep -q ${xml_pkg}

            echo '==> Installing runtime dependencies'
            apt-get install -y -qq ${xml_pkg} ${fwupd_pkg} libglib2.0-0 libmbim-glib4 libudev1 >/dev/null

            echo '==> Unpacking deb (skip postinst systemctl in container)'
            dpkg --unpack /tmp/package.deb

            echo '==> Checking shared library resolution'
            for bin in \
                /opt/rolling/rolling_flash_service/rolling_flash \
                /opt/rolling/rolling_config_service/rolling_config \
                /opt/rolling/rolling_helper_service/rolling_helper; do
                echo \"--- ldd \${bin}\"
                ldd \"\${bin}\"
                if ldd \"\${bin}\" | grep -q 'not found'; then
                    echo \"ERROR: missing shared libraries for \${bin}\" >&2
                    exit 1
                fi
            done

            if [ -f /opt/rolling/rolling_ma_service/rolling_ma ]; then
                echo '--- ldd /opt/rolling/rolling_ma_service/rolling_ma'
                ldd /opt/rolling/rolling_ma_service/rolling_ma
                if ldd /opt/rolling/rolling_ma_service/rolling_ma | grep -q 'not found'; then
                    echo 'ERROR: missing shared libraries for rolling_ma' >&2
                    exit 1
                fi
            fi

            echo '==> Smoke test passed'
        "
}

build_deb_in_container

DEB_FILE="$(ls -1 "${PROJECT_ROOT}/binary_deb/"*-"${DISTRO_TAG}"_amd64.deb 2>/dev/null | tail -1 || true)"
if [[ -z "${DEB_FILE}" ]]; then
    DEB_FILE="$(ls -1t "${PROJECT_ROOT}/binary_deb/"*.deb 2>/dev/null | head -1 || true)"
fi

if [[ -z "${DEB_FILE}" ]]; then
    echo "ERROR: no deb produced in ${PROJECT_ROOT}/binary_deb/" >&2
    exit 1
fi

echo "==> Produced deb: ${DEB_FILE}"
echo "==> Package metadata:"
dpkg-deb -I "${DEB_FILE}" | sed -n '1,20p'

if [[ "${RUN_SMOKE}" -eq 0 ]]; then
    echo "==> Smoke test skipped (--no-smoke)"
    exit 0
fi

run_smoke_test "${DEB_FILE}"
echo "==> Done: ${DEB_FILE}"
