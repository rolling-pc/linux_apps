#!/bin/bash
# verify-debian-pkg.sh -- pre-flight checks before dpkg-buildpackage.
#
#   bash debian/verify-debian-pkg.sh
#
# Checks the things that otherwise only fail deep inside dpkg-buildpackage.

set -uo pipefail

FAIL=0
ok()   { printf '  \033[32mOK\033[0m   %s\n' "$1"; }
bad()  { printf '  \033[31mFAIL\033[0m %s\n' "$1"; FAIL=1; }
warn() { printf '  \033[33mWARN\033[0m %s\n' "$1"; }

echo "== 1. required files =="
for f in debian/control debian/changelog debian/copyright debian/rules \
         debian/source/format debian/linux-apps.install.in; do
    [ -f "$f" ] && ok "$f" || bad "$f missing"
done

echo
echo "== 2. permissions =="
for f in debian/rules debian/preinst debian/postinst debian/prerm debian/postrm; do
    [ -x "$f" ] && ok "$f executable" || bad "$f not executable (chmod +x $f)"
done
for f in debian/control debian/changelog debian/copyright debian/linux-apps.install.in \
         debian/clean debian/README.Debian debian/source/format; do
    [ -f "$f" ] || continue
    [ -x "$f" ] && bad "$f should not be executable (chmod -x $f)"
done
ok "data files are not executable"

echo
echo "== 3. line endings =="
crlf=""
for f in debian/* debian/source/*; do
    [ -f "$f" ] || continue
    case "$f" in *.sh) continue ;; esac
    if od -An -c "$f" 2>/dev/null | tr -d ' \n' | grep -q '\\r'; then
        crlf="$crlf $f"
    fi
done
[ -n "$crlf" ] && bad "CRLF in:$crlf" || ok "all debian/ files use LF"

echo
echo "== 4. changelog =="
if command -v dpkg-parsechangelog >/dev/null; then
    if dpkg-parsechangelog -SVersion >/dev/null 2>&1; then
        ok "dpkg-parsechangelog version=$(dpkg-parsechangelog -SVersion) dist=$(dpkg-parsechangelog -SDistribution)"
    else
        bad "dpkg-parsechangelog failed -- malformed trailer"
    fi
    m=$(dpkg-parsechangelog -SMaintainer 2>/dev/null || true)
    case "$m" in
        *\<*@*.*\>*) ok "Maintainer email: $m" ;;
        *) bad "Maintainer email unparseable: '$m'" ;;
    esac
else
    warn "dpkg-parsechangelog unavailable"
fi
tl=$(grep -n '^ -- ' debian/changelog | tail -1 | cut -d: -f1)
if [ -n "$tl" ]; then
    l=$(sed -n "${tl}p" debian/changelog)
    case "$l" in
        ' -- '*' <'*'>  '*) ok "trailer is ' -- Name <email>  Date' (two spaces)" ;;
        *) bad "trailer malformed (need two spaces before the date): $l" ;;
    esac
else
    bad "no trailer line"
fi

echo
echo "== 5. control =="
grep -q '^Source:' debian/control && ok "Source" || bad "Source missing"
grep -qE '^Maintainer: *[^<[:space:]][^<]*<[^>]+@[^>]+>' debian/control \
    && ok "Maintainer is 'Name <email>'" \
    || bad "Maintainer needs a display name (Policy 5.6.1): 'Name <email>'"
grep -qE '^Standards-Version: *4\.' debian/control && ok "Standards-Version" || bad "Standards-Version missing"
grep -qE '^Homepage: *https?://' debian/control && ok "Homepage" || bad "Homepage missing"
grep -qE '^[A-Za-z-]+:.*,[^ ]' debian/control && bad "comma with no space in control" || ok "comma spacing"
if grep -nE '<(url://example|insert|preferred|nnnn)[^>]*>' debian/control debian/copyright debian/changelog 2>/dev/null; then
    bad "placeholder text present"
else
    ok "no placeholders"
fi

echo
echo "== 6. source format =="
fmt=$(cat debian/source/format 2>/dev/null)
ver=$(dpkg-parsechangelog -SVersion 2>/dev/null || echo "")
case "$fmt" in
    '3.0 (native)')
        ok "3.0 (native) -- self-contained tree, no orig tarball needed"
        case "$ver" in
            *-*) bad "native package version must not carry a Debian revision: $ver" ;;
            '')  warn "could not read the version from debian/changelog" ;;
            *)   ok "version $ver has no Debian revision" ;;
        esac
        # The tarball name and the CMake project version must agree, otherwise
        # the .deb says 2.1.9 while the binaries report something else.
        cmver=$(sed -n 's/^project([^)]*VERSION[[:space:]]\+\([0-9][0-9.]*\).*/\1/p' CMakeLists.txt | head -1)
        if [ -n "$cmver" ] && [ -n "$ver" ]; then
            [ "$cmver" = "$ver" ] && ok "CMakeLists.txt project VERSION $cmver == changelog" \
                                  || bad "CMakeLists.txt VERSION $cmver != changelog $ver"
        fi
        # Neither an orig tarball nor a watch file is meaningful here.
        [ -e debian/watch ] && warn "debian/watch is pointless for a native package" \
                            || ok "no debian/watch (native package)"
        ;;
    '3.0 (quilt)')
        ok "3.0 (quilt)"
        case "$ver" in
            *-*) ;;
            *) bad "quilt package version needs a Debian revision (e.g. 2.1.9-1): $ver" ;;
        esac
        ;;
    *) bad "unexpected format: $fmt" ;;
esac

echo
echo "== 7. rules =="
grep -q 'override_dh_auto_clean' debian/rules && ok "clean override" || warn "no clean override"
if awk '/^[a-zA-Z_%].*:/{t=1;next} /^$/{t=0} t && /^[ ]+[^ ]/ {print NR": "$0}' debian/rules | grep -q .; then
    bad "space-indented recipe line(s) -- make requires TAB:"
    awk '/^[a-zA-Z_%].*:/{t=1;next} /^$/{t=0} t && /^[ ]+[^ ]/ {print "       "NR": "$0}' debian/rules
else
    ok "recipes use tabs"
fi
grep -q 'CMAKE_INSTALL_PREFIX=/' debian/rules && ok "install prefix set to /" || warn "check CMAKE_INSTALL_PREFIX"
if grep -qE '^ifneq' debian/rules; then
    ok "PROJECT/OEM also settable via DEB_BUILD_OPTIONS (make conditionals present)"
fi
if grep -qE ' *= *\$\(filter |:= *\$\(patsubst' debian/rules; then
    ok "DEB_BUILD_OPTIONS parsed with filter/patsubst"
fi
if grep -qE '^PROJECT \?=' debian/rules && grep -qE '^OEM +\?=' debian/rules; then
    ok 'PROJECT/OEM overridable via --eval=PROJECT:=... (make command-line variable)'
else
    warn "PROJECT/OEM defaults not found in debian/rules"
fi
# The ambiguous "-Dproject=" form must not be *recommended* anywhere.  debian/rules
# mentions it once inside an explanatory comment telling the reader NOT to use
# it, so filter comment lines out before looking.
if grep -vE '^[[:space:]]*#' debian/rules debian/README.Debian debian/build-deb.sh 2>/dev/null \
   | grep -qE '\-Dproject=|\-Doem='; then
    bad "documented -Dproject=/-Doem= form is ambiguous: make reads -D as --debug"
else
    ok "no ambiguous -Dproject=/-Doem= documented (use --eval=PROJECT:=...)"
fi

echo
echo "== 8. cmake install destinations must be relative =="
if grep -nE 'DESTINATION *"\$\{CMAKE_INSTALL_PREFIX\}|DESTINATION *"/' CMakeLists.txt; then
    bad "absolute DESTINATION found -- DESTDIR will be ignored"
else
    ok "no absolute install DESTINATION"
fi

# install(DIRECTORY) only accepts permission tokens after FILE_PERMISSIONS or
# DIRECTORY_PERMISSIONS.  A bare "OWNER_EXECUTE OWNER_READ ..." is parsed as one
# more source directory and the build stops with
#   file INSTALL cannot find ".../OWNER_EXECUTE"
# which is exactly what broke the CPack run on 2026-10-21.
perm_report=$(
    find . -name CMakeLists.txt -not -path "./application/3rd/*" -print0 \
    | xargs -0 awk '
        { line = $0; sub(/#.*/, "", line) }
        {
            if (inb == 0 && line ~ /install\(DIRECTORY/) { inb = 1; blk = line }
            else if (inb == 1) { blk = blk "\n" line }
            else next
            if (line ~ /\)/) { inb = 0; n++; B[n] = blk; F[n] = FILENAME; L[n] = FNR }
        }
        END {
            for (i = 1; i <= n; i++) {
                if (B[i] ~ /PERMISSIONS/) continue
                if (B[i] ~ /(OWNER|GROUP|WORLD)_(EXECUTE|WRITE|READ)/)
                    printf "%s:%d: bare permission token in install(DIRECTORY)\n", F[i], L[i]
            }
        }'
)
if [ -n "$perm_report" ]; then
    printf '%s\n' "$perm_report" | while IFS= read -r l; do bad "$l"; done
else
    ok "no bare permission tokens in install(DIRECTORY) (they would be read as source paths)"
fi

echo
echo "== 9. debian/linux-apps.install.in covers the staged tree =="
# Every source path listed in debian/linux-apps.install.in that has no glob must be a path
# the upstream install helper can produce.  We cannot run the build here, so
# check the two structural invariants that used to break the build:
# The template lists every path any supported (project, OEM) pair can produce;
# debian/rules prunes what did not match and materialises empty directories.
#   a) a bare directory must never be listed -- dh_install would try to copy it
#      and dh_missing/empty-dir handling would diverge from the CPack payload;
#   b) the empty drop directory must use the glob form, which the generator
#      turns into an empty directory in the package tree.
if grep -qE '^opt/rolling/rolling_fw_pkg( |$)' debian/linux-apps.install.in; then
    bad "manifest lists the bare directory opt/rolling/rolling_fw_pkg -- dh_install cannot ship a directory"
elif grep -qE '^opt/rolling/rolling_fw_pkg/\*' debian/linux-apps.install.in; then
    ok "empty drop directory uses the glob form that debian/rules materialises"
else
    bad "manifest has no entry for opt/rolling/rolling_fw_pkg"
fi
if grep -qE '^opt/rolling/rolling_ma_service/\*' debian/linux-apps.install.in; then
    ok "MA helper is in the manifest (pruned automatically when not staged)"
else
    bad "manifest has no entry for the MA helper -- its files would be unclaimed"
fi
if grep -q 'done < debian/linux-apps.install.in' debian/rules; then
    ok "debian/rules generates debian/linux-apps.install from the template"
else
    bad "debian/rules does not generate the install list from the template"
fi
if [ -e debian/install ]; then
    bad "debian/install exists -- dh_install would have two competing lists"
else
    ok "no static debian/install to shadow the generated list"
fi
if [ -d debian/tmp ]; then
    miss=0
    while read -r s d; do
        [ -z "$s" ] && continue
        case "$s" in \#*) continue ;; esac
        case "$s" in
            *'*'*|*'?'*|*'['*) ls -d debian/tmp/$s >/dev/null 2>&1 || continue ;;
            *) if ! ls -d "debian/tmp/$s" >/dev/null 2>&1; then
                   echo "       not staged: debian/tmp/$s"
                   miss=1
               fi ;;
        esac
    done < debian/linux-apps.install.in
    [ "$miss" -eq 0 ] && ok "all plain debian/linux-apps.install.in sources exist under debian/tmp" \
                      || bad "debian/linux-apps.install.in references paths missing from debian/tmp"

    # compat 13 makes dh_missing default to --fail-missing: every staged file
    # must be claimed by some dh tool or listed in debian/not-installed.
    leftover=0
    while IFS= read -r p; do
        rel=${p#debian/tmp/}
        claimed=0
        while read -r s d; do
            [ -z "$s" ] && continue
            case "$s" in \#*) continue ;; esac
            # shellcheck disable=SC2254
            case "$rel" in $s) claimed=1; break ;; esac
        done < debian/linux-apps.install.in
        [ "$claimed" -eq 1 ] && continue
        if [ -f debian/not-installed ]; then
            while read -r s; do
                [ -z "$s" ] && continue
                case "$s" in \#*) continue ;; esac
                # shellcheck disable=SC2254
                case "$rel" in $s) claimed=1; break ;; esac
            done < debian/not-installed
        fi
        [ "$claimed" -eq 1 ] && continue
        echo "       unclaimed staged file: $rel"
        leftover=1
    done < <(find debian/tmp -type f 2>/dev/null)
    [ "$leftover" -eq 0 ] && ok "no staged file is left uninstalled (dh_missing safe)" \
                          || bad "staged files not claimed by any dh tool -- dh_missing will fail (add to debian/not-installed)"
else
    warn "debian/tmp absent (expected before a build)"
fi

echo
echo "== 10. parity with the CPack build =="
# debian/<script> must be a byte-for-byte copy of script/<script>, because
# script/ is what CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA embeds.
for s in preinst postinst prerm postrm; do
    if [ ! -f "script/$s" ]; then
        warn "script/$s missing"
    elif cmp -s "script/$s" "debian/$s"; then
        ok "debian/$s is identical to script/$s"
    else
        bad "debian/$s differs from script/$s -- run: cp script/$s debian/$s"
    fi
done

# Anything that rewrites an ELF file after installation makes the debhelper
# payload differ from the CPack payload.
for t in dh_strip dh_dwz dh_installsystemd; do
    if grep -qE "^override_${t}:[[:space:]]*$" debian/rules; then
        ok "$t disabled (payload parity)"
    else
        bad "$t not disabled -- the two payloads would diverge"
    fi
done

if grep -vE '^[[:space:]]*#' CMakeLists.txt | grep -q 'FILE_PERMISSIONS'; then
    bad "CMakeLists.txt sets FILE_PERMISSIONS -- it overrides the normalised staged modes on one path only"
else
    ok "no FILE_PERMISSIONS in CMakeLists.txt"
fi
if grep -q 'normalize_permissions' install; then
    ok "install helper normalises staging modes (0755/0644)"
else
    bad "install helper does not normalise modes -- permissions may differ between the two paths"
fi
# The license claimed for the package and the license in the per-file headers
# must agree, otherwise debian/copyright is simply wrong.
if awk '/^Files: \*$/{f=1;next} /^Files:/{f=0} f && /^License: LGPL-2\.1$/{found=1} END{exit !found}' debian/copyright; then
    ok "copyright licenses Files: * as LGPL-2.1"
else
    bad "copyright 'Files: *' is not LGPL-2.1"
fi
if [ -f LICENSE ] && grep -qi 'LESSER GENERAL PUBLIC LICENSE' LICENSE 2>/dev/null; then
    ok "top-level LICENSE contains the LGPL-2.1 text (SECTION A)"
else
    bad "top-level LICENSE is missing the LGPL-2.1 text (SECTION A)"
fi
if [ -f LICENSE ] && grep -q 'SECTION B: CUSTOM BINARY REDISTRIBUTION LICENSE' LICENSE 2>/dev/null; then
    ok "LICENSE SECTION B grants redistribution of the binary_deb/ prebuilt artefacts"
else
    bad "LICENSE is missing SECTION B -- binary_deb/ has no redistribution grant"
fi
if grep -q 'License: Rolling-binary-1\.0' debian/copyright && \
   awk '/^Files: binary_deb/{f=1;next} /^Files:/{f=0} f && /^License: Rolling-binary-1\.0$/{found=1} END{exit !found}' debian/copyright; then
    ok "copyright licenses binary_deb/* as Rolling-binary-1.0 (matches LICENSE SECTION B)"
else
    bad "copyright does not license binary_deb/* as Rolling-binary-1.0"
fi
# Each group of prebuilt artefacts lives in its own directory and carries its
# own self-contained copy of the grant, so that a directory copied out of the
# tree (which is how these files are consumed) still comes with the terms.
ARTEFACT_DIRS="binary_deb/common_lib application/rolling_ma_service"
for d in $ARTEFACT_DIRS; do
    if [ -f "$d/LICENSE" ]; then
        ok "$d/LICENSE present (standalone binary redistribution grant)"
    else
        bad "$d/LICENSE missing -- the prebuilt artefacts carry no license"
    fi
    grep -q 'CUSTOM BINARY REDISTRIBUTION LICENSE' "$d/LICENSE" 2>/dev/null \
        && ok "$d/LICENSE is the custom binary redistribution grant" \
        || bad "$d/LICENSE does not look like the binary redistribution grant"
    grep -q 'SHA-256' "$d/LICENSE" 2>/dev/null \
        && ok "$d/LICENSE pins its artefacts by SHA-256"
done
# The archives and rolling_ma are disjoint, so each license must cover exactly
# its own directory: everything present here, nothing that lives elsewhere.
missing=""
for f in libcode.a libmtk_rw350r_dell_2.0.25.a librolling_config_service_2.0.11.a \
         librolling_flash_service_2.0.19.a; do
    grep -q -- "$f" binary_deb/common_lib/LICENSE 2>/dev/null || missing="$missing $f"
done
[ -z "$missing" ] && ok "common_lib/LICENSE names all four static archives" \
                  || bad "common_lib/LICENSE does not name:$missing"
if grep -q 'rolling_ma' binary_deb/common_lib/LICENSE 2>/dev/null; then
    bad "common_lib/LICENSE still mentions rolling_ma -- it now lives in application/rolling_ma_service/"
else
    ok "common_lib/LICENSE does not claim rolling_ma"
fi
grep -q 'rolling_ma' application/rolling_ma_service/LICENSE 2>/dev/null \
    && ok "rolling_ma_service/LICENSE covers rolling_ma" \
    || bad "rolling_ma_service/LICENSE does not mention rolling_ma"
if grep -q 'dmidecode' application/rolling_ma_service/LICENSE 2>/dev/null; then
    ok "rolling_ma_service/LICENSE carries the dmidecode (GPL-2) note"
else
    bad "rolling_ma_service/LICENSE is missing the dmidecode note"
fi
# The build helper has to find rolling_ma at its new location.
if grep -q 'MA_SERVICE_PATH_APP_DIR}/rolling_ma' install; then
    ok "install resolves rolling_ma from application/rolling_ma_service/"
else
    bad "install does not look for rolling_ma in application/rolling_ma_service/ -- MA targets will fail"
fi

# Every .c/.h of ours must carry the same header text as debian/copyright.
gpl_headers=$(grep -rl 'GNU General Public License version' \
                --include='*.c' --include='*.h' application/ 2>/dev/null \
              | grep -v '/3rd/' | grep -v 'rolling_list\.h' | wc -l)
lgpl_headers=$(grep -rl 'GNU Lesser General Public License' \
                 --include='*.c' --include='*.h' application/ 2>/dev/null \
               | grep -v '/3rd/' | wc -l)
total=$(( $(find application -name '*.c' -o -name '*.h' 2>/dev/null | grep -v '/3rd/' | wc -l) ))
if [ "$gpl_headers" -eq 0 ] && [ "$lgpl_headers" -eq $(( total - 1 )) ]; then
    ok "all $lgpl_headers own source files carry an LGPL-2.1 header (rolling_list.h excepted)"
else
    bad "license headers inconsistent: $gpl_headers still say GPL-2, $lgpl_headers say LGPL-2.1, $(( total - 1 )) expected"
fi

# The prebuilt artefacts are x86-64 only, so the package cannot claim "any".
if grep -qE '^Architecture: amd64$' debian/control; then
    ok "Architecture: amd64 (matches the x86-64 prebuilt libraries)"
else
    warn "Architecture is not amd64 -- binary_deb/ only ships x86-64 objects"
fi

echo
if [ "$FAIL" -eq 0 ]; then
    printf '\033[32mAll blocking checks passed.\033[0m\n'
    echo "Next: dpkg-buildpackage -us -uc -b"
else
    printf '\033[31mFix the failures above before building.\033[0m\n'
fi
exit "$FAIL"
