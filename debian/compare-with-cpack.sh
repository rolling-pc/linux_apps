#!/bin/sh
# compare-with-cpack.sh -- prove that the debhelper build produces the same
# package as the CPack build.
#
#   ./debian/compare-with-cpack.sh [project] [oem]
#   ./debian/compare-with-cpack.sh rw350r dell
#
# It builds both ways, unpacks the two .deb files and compares:
#   1. the maintainer scripts (preinst/postinst/prerm/postrm), byte for byte
#   2. the payload: every path, its type (file/dir/link), its mode, and the
#      md5sum of every file
#
# /usr/share/doc/linux-apps is excluded from the payload comparison: Policy
# 12.5 requires it and CPack does not create it, so it is the one intentional
# difference.  Everything else must match exactly.
#
# Exit status: 0 when identical, 1 on any difference.

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
SRC=$(cd "${SCRIPT_DIR}/.." && pwd)

PROJECT=${1:-rw350r}
OEM=${2:-dell}

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

say() { printf '==> %s\n' "$*"; }
die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

cd "$SRC"

# ---------------------------------------------------------------- build cpack
say "[1/5] cpack build (${PROJECT}/${OEM})"
rm -rf build
cmake -S . -B build \
      -DBUILD_DEB=yes -DPROJECT_BUILD="$PROJECT" -DOEM_BUILD="$OEM" \
      -DBUILD_BY_LIB=1 > "$WORK/cmake.log" 2>&1 || {
    tail -n 40 "$WORK/cmake.log" >&2; die "cmake configure failed"; }
cmake --build build >> "$WORK/cmake.log" 2>&1 || {
    tail -n 40 "$WORK/cmake.log" >&2; die "build failed"; }
( cd build && cpack >> "$WORK/cmake.log" 2>&1 ) || {
    tail -n 40 "$WORK/cmake.log" >&2; die "cpack failed"; }

CPACK_DEB=$(ls -1t "$SRC"/build/*.deb 2>/dev/null | head -n 1)
[ -n "$CPACK_DEB" ] || die "cpack produced no .deb"
say "      $(basename "$CPACK_DEB")"

# ---------------------------------------------------------------- build dpkg
say "[2/5] debhelper build"
rm -rf build
if command -v fakeroot >/dev/null 2>&1; then
    fakeroot dpkg-buildpackage -us -uc -b > "$WORK/dpkg.log" 2>&1 || {
        tail -n 60 "$WORK/dpkg.log" >&2; die "dpkg-buildpackage failed"; }
else
    dpkg-buildpackage -us -uc -b > "$WORK/dpkg.log" 2>&1 || {
        tail -n 60 "$WORK/dpkg.log" >&2; die "dpkg-buildpackage failed"; }
fi

DPKG_DEB=$(ls -1t "$SRC"/../linux-apps_*_"$(dpkg --print-architecture)".deb 2>/dev/null | head -n 1)
[ -n "$DPKG_DEB" ] || DPKG_DEB=$(ls -1t "$SRC"/../linux-apps_*.deb 2>/dev/null | head -n 1)
[ -n "$DPKG_DEB" ] || die "dpkg-buildpackage produced no .deb"
say "      $(basename "$DPKG_DEB")"

# ---------------------------------------------------------------- unpack both
say "[3/5] unpacking"
mkdir -p "$WORK/cpack/root" "$WORK/cpack/control" "$WORK/dpkg/root" "$WORK/dpkg/control"
dpkg-deb -x "$CPACK_DEB" "$WORK/cpack/root"
dpkg-deb -e "$CPACK_DEB" "$WORK/cpack/control"
dpkg-deb -x "$DPKG_DEB" "$WORK/dpkg/root"
dpkg-deb -e "$DPKG_DEB" "$WORK/dpkg/control"

# ---------------------------------------------------------------- scripts
say "[4/5] maintainer scripts"
rc=0
for s in preinst postinst prerm postrm; do
    if [ -f "$WORK/cpack/control/$s" ] || [ -f "$WORK/dpkg/control/$s" ]; then
        if cmp -s "$WORK/cpack/control/$s" "$WORK/dpkg/control/$s"; then
            printf '      same    %s\n' "$s"
        else
            printf '      DIFFER  %s\n' "$s"
            diff -u "$WORK/cpack/control/$s" "$WORK/dpkg/control/$s" | sed 's/^/              /' || true
            rc=1
        fi
    else
        printf '      absent  %s (both)\n' "$s"
    fi
done

# ---------------------------------------------------------------- payload
say "[5/5] payload (paths, types, modes, md5sums)"
snapshot() {
    root="$1"
    out="$2"
    : > "$out"
    (
        cd "$root"
        find . -mindepth 1 \
             ! -path './usr/share/doc/linux-apps' \
             ! -path './usr/share/doc/linux-apps/*' \
             -printf '%y %m %p\n' | sort
        find . -type f \
             ! -path './usr/share/doc/linux-apps/*' \
             -exec md5sum {} + | sort -k 2
    ) >> "$out"
}

snapshot "$WORK/cpack/root" "$WORK/cpack.txt"
snapshot "$WORK/dpkg/root"  "$WORK/dpkg.txt"

if diff -u "$WORK/cpack.txt" "$WORK/dpkg.txt" > "$WORK/payload.diff"; then
    printf '      identical: %s entries\n' "$(wc -l < "$WORK/cpack.txt")"
else
    printf '      DIFFER:\n'
    sed 's/^/              /' "$WORK/payload.diff"
    rc=1
fi

echo
if [ "$rc" -eq 0 ]; then
    echo "RESULT: the two packages are identical (apart from /usr/share/doc/linux-apps)."
else
    echo "RESULT: differences found; see above."
fi
exit "$rc"
