#!/bin/bash
# build-deb.sh -- one-shot wrapper around dpkg-buildpackage.
#
#   ./debian/build-deb.sh [PROJECT] [OEM]
#
# Defaults: rw350r dell
#
# PROJECT / OEM are forwarded to debian/rules as make command-line variables,
# which take precedence over the "?=" defaults there.

set -euo pipefail

PROJECT="${1:-rw350r}"
OEM="${2:-dell}"

cd "$(dirname "$0")/.."
ROOT="$(pwd)"

echo "############################################################"
echo "# linux-apps deb builder"
echo "#   project : ${PROJECT}"
echo "#   oem     : ${OEM}"
echo "#   root    : ${ROOT}"
echo "############################################################"
echo

echo "==> [1/4] pre-flight checks"
bash debian/verify-debian-pkg.sh || { echo; echo "Fix the issues above first." >&2; exit 1; }

echo
echo "==> [2/4] cleaning"
rm -rf build debian/tmp debian/files
rm -f debian/linux-apps.substvars
find debian -name '*.debhelper*' -delete 2>/dev/null || true

echo
echo "==> [3/4] dpkg-buildpackage -b (project=${PROJECT} oem=${OEM})"
# "--eval='VAR:=value'" is passed straight through to make as a command-line
# variable, which overrides the "?=" defaults in debian/rules.  This avoids the
# ambiguous -D form, which make reads as --debug.
dpkg-buildpackage -us -uc -b "--eval=PROJECT:=${PROJECT}" "--eval=OEM:=${OEM}"

echo
echo "==> [4/4] result"
DEB=$(find "${ROOT}/.." -maxdepth 1 -name 'linux-apps_*.deb' -newermt '-10 minutes' -print -quit 2>/dev/null || true)
[ -n "${DEB}" ] || DEB=$(ls -1t "${ROOT}/../linux-apps_"*.deb 2>/dev/null | head -1 || true)
if [ -z "${DEB}" ]; then
    echo "No .deb produced -- inspect the build log above." >&2
    exit 1
fi

echo "Package: ${DEB}"
echo
echo "--- control ---"
dpkg-deb -I "${DEB}" | head -30
echo
echo "--- files ---"
dpkg-deb -c "${DEB}" | awk '{print $1, $6, $7, $8, $9}'
echo
echo "--- ldd ---"
TMP=$(mktemp -d)
dpkg-deb -x "${DEB}" "${TMP}"
find "${TMP}"/opt/rolling -type f -perm -u+x 2>/dev/null | while read -r b; do
    case "$b" in *.ini|*.rules|*.conf) continue ;; esac
    echo "-- ${b#"${TMP}"}"
    if ldd "$b" 2>/dev/null | grep -q 'not found'; then
        ldd "$b" | grep 'not found' | sed 's/^/   MISSING: /'
    else
        echo "   ok"
    fi
done
rm -rf "${TMP}"
echo
echo "Done."
