#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
build=$1
expected_version=$2
pc=$build/meson-private/libpkgreconcile-apply-posix.pc
fail() { echo "pkgconfig-metadata-contract: $*" >&2; [ ! -f "$pc" ] || cat "$pc" >&2; exit 1; }
[ -s "$pc" ] || fail 'generated libpkgreconcile-apply-posix.pc is missing'
version=$(sed -n 's/^Version:[[:space:]]*//p' "$pc")
[ "$version" = "$expected_version" ] || fail "version is '$version', expected '$expected_version'"
requires=$(sed -n 's/^Requires:[[:space:]]*//p' "$pc")
for required in \
  'libpkgreconcile-apply >= 0.1.0' \
  'libpkgapply-posix >= 3.2.0' \
  'libpkgreconcile-posix >= 0.1.0'; do
  printf '%s\n' "$requires" | grep -F "$required" >/dev/null || fail "missing public dependency: $required"
done
if printf '%s\n' "$requires" | grep -E 'libpkgstate|pkgctl|libpkgtransaction' >/dev/null; then
  fail 'state/controller/transaction dependency leaked into pkg-config'
fi
printf '%s\n' 'pkgconfig-metadata-contract: ok'
