#!/bin/bash
# Build linux-apps static libraries or deb.
#
# Rule:
#   Ubuntu 26.04 (resolute) -> build inside Docker
#   Ubuntu 22.04 / 24.04    -> build on host via ./script/make_deb.sh
#
# Usage:
#   ./script/build_multi_distro.sh lib  <project> <oem> <distro>
#   ./script/build_multi_distro.sh deb  <project> <oem> <distro> [--by-lib] [--no-smoke]
#
# Examples:
#   ./script/build_multi_distro.sh deb  rw350r dell noble          # host make_deb.sh
#   ./script/build_multi_distro.sh deb  rw350r dell jammy          # host make_deb.sh
#   ./script/build_multi_distro.sh deb  rw350r dell resolute       # Docker
#   ./script/build_multi_distro.sh lib  rw350r dell resolute       # Docker
#
# Distro aliases:
#   jammy     -> ubuntu22.04 (host)
#   noble     -> ubuntu24.04 (host)
#   resolute  -> ubuntu26.04 (docker)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
DOCKER_DIR="${PROJECT_ROOT}/docker"

PKG_TYPE="${1:-}"
PROJECT="${2:-}"
OEM="${3:-}"
DISTRO="${4:-}"
BUILD_BY_LIB=0
RUN_SMOKE=1
USE_DOCKER=0

usage() {
    cat <<EOF
Usage:
  $0 lib <project> <oem> <distro>
  $0 deb <project> <oem> <distro> [--by-lib] [--no-smoke]

Distro values:
  jammy     Ubuntu 22.04  -> host ./script/make_deb.sh  (common_lib-ubuntu22.04/)
  noble     Ubuntu 24.04  -> host ./script/make_deb.sh  (common_lib-ubuntu24.04/)
  resolute  Ubuntu 26.04  -> Docker                     (common_lib-ubuntu26.04/)

Examples:
  $0 deb rw350r dell noble
  $0 deb rw350r dell resolute --by-lib
EOF
}

resolve_distro() {
    case "$1" in
        jammy|ubuntu22.04|22.04)
            DOCKERFILE="Dockerfile.jammy"
            BASE_IMAGE="ubuntu:22.04"
            DISTRO_TAG="ubuntu22.04"
            USE_DOCKER=0
            ;;
        noble|ubuntu24.04|24.04)
            DOCKERFILE="Dockerfile.noble"
            BASE_IMAGE="ubuntu:24.04"
            DISTRO_TAG="ubuntu24.04"
            USE_DOCKER=0
            ;;
        resolute|ubuntu26.04|26.04)
            DOCKERFILE="Dockerfile.resolute"
            BASE_IMAGE="ubuntu:26.04"
            DISTRO_TAG="ubuntu26.04"
            USE_DOCKER=1
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

if [[ "$PKG_TYPE" != "lib" && "$PKG_TYPE" != "deb" ]]; then
    echo "Unknown command: $PKG_TYPE (expected lib or deb)" >&2
    usage
    exit 1
fi

resolve_distro "$DISTRO"

IMAGE_NAME="rolling-linux-apps-build:${DISTRO_TAG}"
SMOKE_IMAGE_NAME="rolling-linux-apps-smoke:${DISTRO_TAG}"

prepare_common_lib_for_by_lib() {
    if [[ "${BUILD_BY_LIB}" -ne 1 ]]; then
        return 0
    fi

    if [[ ! -d "${LIB_OUTPUT_DIR}" ]]; then
        echo "ERROR: ${LIB_OUTPUT_DIR} not found." >&2
        echo "Run first: $0 lib ${PROJECT} ${OEM} ${DISTRO}" >&2
        exit 1
    fi

    if [[ "${PROJECT}" == "rw350r" && "${OEM}" == "dell" ]] && [[ ! -f "${LIB_OUTPUT_DIR}/rolling_ma" ]]; then
        echo "ERROR: ${LIB_OUTPUT_DIR}/rolling_ma not found." >&2
        echo "rolling_ma must be built in the same Ubuntu version as the deb target." >&2
        echo "Run first: $0 lib ${PROJECT} ${OEM} ${DISTRO}" >&2
        exit 1
    fi

    echo "==> Will use prebuilt libs from ${LIB_OUTPUT_DIR}"
}

verify_rolling_ma_for_rw350r_dell() {
    if [[ "${PROJECT}" != "rw350r" || "${OEM}" != "dell" ]]; then
        return 0
    fi

    local ma_path="$1"
    if [[ ! -f "${ma_path}" ]]; then
        echo "ERROR: rolling_ma missing at ${ma_path}" >&2
        echo "rolling_ma must be compiled in ${DISTRO_TAG} and packaged into the deb." >&2
        exit 1
    fi

    echo "==> Verified rolling_ma: ${ma_path} ($(stat -c%s "${ma_path}") bytes)"
}

ensure_docker_image() {
    echo "==> Building image ${IMAGE_NAME} from ${DOCKERFILE}"
    docker build -f "${DOCKER_DIR}/${DOCKERFILE}" -t "${IMAGE_NAME}" "${DOCKER_DIR}"
}

build_libs_in_container() {
    echo "==> Building static libraries in Docker (${DISTRO_TAG})"
    docker run --rm \
        -e HOST_UID="$(id -u)" \
        -e HOST_GID="$(id -g)" \
        -e DISTRO_TAG="${DISTRO_TAG}" \
        -v "${PROJECT_ROOT}:/workspace" \
        -w /workspace \
        "${IMAGE_NAME}" \
        bash -lc "
            set -euo pipefail
            if [ -d build ]; then rm -rf build; fi
            mkdir -p binary_deb
            ./script/make_deb.sh lib ${PROJECT} ${OEM}
            rm -rf /workspace/binary_deb/common_lib-${DISTRO_TAG}
            cp -a /workspace/binary_deb/common_lib /workspace/binary_deb/common_lib-${DISTRO_TAG}
            chown -R \"\${HOST_UID}:\${HOST_GID}\" /workspace/binary_deb/common_lib /workspace/binary_deb/common_lib-${DISTRO_TAG} 2>/dev/null || true
        "

    verify_rolling_ma_for_rw350r_dell "${LIB_OUTPUT_DIR}/rolling_ma"

    if [[ ! -d "${LIB_OUTPUT_DIR}" ]]; then
        echo "ERROR: lib build did not produce ${LIB_OUTPUT_DIR}" >&2
        exit 1
    fi

    echo "==> Libraries installed to ${LIB_OUTPUT_DIR}"
    ls -la "${LIB_OUTPUT_DIR}"
}

build_deb_in_container() {
    prepare_common_lib_for_by_lib

    local by_lib_arg=""
    if [[ "${BUILD_BY_LIB}" -eq 1 ]]; then
        by_lib_arg="--by-lib"
    fi

    echo "==> Building deb in Docker via make_deb.sh (${DISTRO_TAG})"
    docker run --rm \
        -e HOST_UID="$(id -u)" \
        -e HOST_GID="$(id -g)" \
        -e DISTRO_TAG="${DISTRO_TAG}" \
        -v "${PROJECT_ROOT}:/workspace" \
        -w /workspace \
        "${IMAGE_NAME}" \
        bash -lc "
            set -euo pipefail
            if [ ${BUILD_BY_LIB} -eq 1 ]; then
                rm -rf /workspace/binary_deb/common_lib
                cp -a /workspace/binary_deb/common_lib-${DISTRO_TAG} /workspace/binary_deb/common_lib
            fi
            ./script/make_deb.sh deb ${PROJECT} ${OEM} ${by_lib_arg}
            chown -R \"\${HOST_UID}:\${HOST_GID}\" /workspace/binary_deb /workspace/build 2>/dev/null || true
        "

    verify_rolling_ma_for_rw350r_dell "${PROJECT_ROOT}/build/application/rolling_ma_service/rolling_ma"
    if [[ "${BUILD_BY_LIB}" -eq 1 ]] || [[ -f "${LIB_OUTPUT_DIR}/rolling_ma" ]]; then
        verify_rolling_ma_for_rw350r_dell "${LIB_OUTPUT_DIR}/rolling_ma"
    fi
}

build_libs_on_host() {
    echo "==> Building static libraries on host via make_deb.sh (${DISTRO_TAG})"
    echo "==> Tip: host Ubuntu should match ${DISTRO_TAG}"
    export DISTRO_TAG
    (
        cd "${PROJECT_ROOT}"
        if [[ -d build ]]; then rm -rf build; fi
        mkdir -p binary_deb
        ./script/make_deb.sh lib "${PROJECT}" "${OEM}"
        rm -rf "binary_deb/common_lib-${DISTRO_TAG}"
        cp -a binary_deb/common_lib "binary_deb/common_lib-${DISTRO_TAG}"
    )

    verify_rolling_ma_for_rw350r_dell "${LIB_OUTPUT_DIR}/rolling_ma"

    if [[ ! -d "${LIB_OUTPUT_DIR}" ]]; then
        echo "ERROR: lib build did not produce ${LIB_OUTPUT_DIR}" >&2
        exit 1
    fi

    echo "==> Libraries installed to ${LIB_OUTPUT_DIR}"
    ls -la "${LIB_OUTPUT_DIR}"
}

build_deb_on_host() {
    prepare_common_lib_for_by_lib

    local by_lib_arg=""
    if [[ "${BUILD_BY_LIB}" -eq 1 ]]; then
        by_lib_arg="--by-lib"
    fi

    echo "==> Building deb on host via make_deb.sh (${DISTRO_TAG})"
    echo "==> Tip: host Ubuntu should match ${DISTRO_TAG}"
    export DISTRO_TAG
    (
        cd "${PROJECT_ROOT}"
        if [[ "${BUILD_BY_LIB}" -eq 1 ]]; then
            rm -rf binary_deb/common_lib
            cp -a "binary_deb/common_lib-${DISTRO_TAG}" binary_deb/common_lib
        fi
        ./script/make_deb.sh deb "${PROJECT}" "${OEM}" ${by_lib_arg}
    )

    verify_rolling_ma_for_rw350r_dell "${PROJECT_ROOT}/build/application/rolling_ma_service/rolling_ma"
    if [[ "${BUILD_BY_LIB}" -eq 1 ]] || [[ -f "${LIB_OUTPUT_DIR}/rolling_ma" ]]; then
        verify_rolling_ma_for_rw350r_dell "${LIB_OUTPUT_DIR}/rolling_ma"
    fi
}

run_smoke_test() {
    local deb_file="$1"
    local xml_pkg="libxml2"
    local fwupd_pkg="libfwupd3"

    if [[ "${DISTRO_TAG}" == "ubuntu26.04" ]]; then
        xml_pkg="libxml2-16"
    fi

    if ! command -v docker >/dev/null 2>&1; then
        echo "==> Smoke test skipped: docker not available"
        return 0
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
            dpkg-deb -I /tmp/package.deb | grep -E '^ Depends:' | grep -q ${fwupd_pkg}

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

# ---------- main ----------
if [[ "${USE_DOCKER}" -eq 1 ]]; then
    echo "==> Target ${DISTRO_TAG}: use Docker"
    ensure_docker_image
    if [[ "$PKG_TYPE" == "lib" ]]; then
        build_libs_in_container
        echo "==> Done: ${LIB_OUTPUT_DIR}"
        exit 0
    fi
    build_deb_in_container
else
    echo "==> Target ${DISTRO_TAG}: use host ./script/make_deb.sh (no Docker compile)"
    if [[ "$PKG_TYPE" == "lib" ]]; then
        build_libs_on_host
        echo "==> Done: ${LIB_OUTPUT_DIR}"
        exit 0
    fi
    build_deb_on_host
fi

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
