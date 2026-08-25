#!/bin/bash
# Thin wrapper around script/build_multi_distro.sh
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/../script/build_multi_distro.sh" "$@"
