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
normalized=$(printf '%s\n' "$requires" |
  tr ',' '\n' |
  sed 's/^[[:space:]]*//; s/[[:space:]]*$//; s/[[:space:]][[:space:]]*/ /g' |
  sed '/^$/d')
count=$(printf '%s\n' "$normalized" | wc -l | tr -d ' ')
[ "$count" -eq 4 ] || fail "public dependency closure has $count clauses, expected 4"
require_once()
{
  clause=$1
  matches=$(printf '%s\n' "$normalized" | grep -Fxc "$clause" || true)
  [ "$matches" -eq 1 ] || fail "dependency clause '$clause' occurs $matches times"
}
require_once 'libpkgreconcile-apply >= 0.1.2'
require_once 'libpkgapply-posix >= 4.0.0'
require_once 'libpkgapply-posix < 5.0.0'
require_once 'libpkgreconcile-posix >= 0.1.0'
if printf '%s\n' "$normalized" | grep -E 'libpkgstate|pkgctl|libpkgtransaction|libpkgapply([[:space:]]|$)' >/dev/null; then
  fail 'state/controller/transaction/core-apply dependency leaked into pkg-config'
fi
printf '%s\n' 'pkgconfig-metadata-contract: ok'
