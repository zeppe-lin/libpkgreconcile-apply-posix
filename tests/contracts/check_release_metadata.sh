#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
fail() { echo "release-metadata-contract: $*" >&2; exit 1; }
grep -F "version: '0.1.1'" "$root/meson.build" >/dev/null || fail 'Meson project version is not 0.1.1'
grep -F "meson_version: '>=1.6.0'" "$root/meson.build" >/dev/null || fail 'Meson floor is not 1.6.0'
grep -F "soversion: '0'" "$root/src/meson.build" >/dev/null || fail 'shared-library ABI generation is not 0'
grep -F 'PROJECT_NUMBER          = 0.1.1' "$root/Doxyfile" >/dev/null || fail 'Doxygen version is stale'
apply_block=$(sed -n '/^libpkgreconcile_apply_dep = dependency(/,/^)/p' "$root/meson.build")
printf '%s
' "$apply_block" | grep -F "version: '>=0.1.1'," >/dev/null || fail 'reconciliation-apply dependency floor is not 0.1.1'
posix_block=$(sed -n '/^libpkgapply_posix_dep = dependency(/,/^)/p' "$root/meson.build")
printf '%s
' "$posix_block" | grep -F "version: '>=3.2.1'," >/dev/null || fail 'application POSIX dependency floor is not 3.2.1'
grep -F '## 0.1.1' "$root/HISTORY.md" >/dev/null || fail '0.1.1 history entry is missing'
[ -s "$root/abi/libpkgreconcile-apply-posix.exports" ] || fail 'reviewed ABI manifest is missing'
printf '%s\n' 'release-metadata-contract: ok'
