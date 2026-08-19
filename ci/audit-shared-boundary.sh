#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
[ "$#" -eq 1 ] || { echo "usage: $0 INSTALLED-LIBRARY" >&2; exit 2; }
library=$1
[ -s "$library" ] || { echo "shared-boundary-audit: missing $library" >&2; exit 1; }
output=$(readelf -d "$library")
printf '%s\n' "$output"
printf '%s\n' "$output" | grep -F 'Library soname: [libpkgreconcile-apply-posix.so.0]' >/dev/null || {
  echo 'shared-boundary-audit: wrong SONAME' >&2
  exit 1
}
needed=$(printf '%s\n' "$output" | grep 'Shared library:' || true)
for dependency in \
  'libpkgreconcile-apply.so.0' \
  'libpkgapply-posix.so.3' \
  'libpkgreconcile-posix.so.0'
do
  printf '%s\n' "$needed" | grep -F "Shared library: [$dependency]" >/dev/null || {
    echo "shared-boundary-audit: missing $dependency" >&2
    exit 1
  }
done
if printf '%s\n' "$needed" | grep -E 'libpkgapply\.so|libpkgstate|libpkgtransaction|pkgctl' >/dev/null; then
  echo 'shared-boundary-audit: redundant owner/controller dependency is direct' >&2
  exit 1
fi
