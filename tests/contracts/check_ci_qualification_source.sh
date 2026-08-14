#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
workflow=$root/.github/workflows/ci.yml
fail() { echo "ci-qualification-source-contract: $*" >&2; exit 1; }
[ -s "$workflow" ] || fail 'CI workflow is missing'
grep -F 'for suite in unit integration header contract' "$root/ci/configure-and-test.sh" >/dev/null ||
  fail 'CI suite loop does not enumerate house suites explicitly'
grep -F -- '--suite "$suite"' "$root/ci/configure-and-test.sh" >/dev/null ||
  fail 'CI suite loop does not execute each named suite'
for mode in shared static; do grep -F "mode: $mode" "$workflow" >/dev/null || fail "CI omits $mode"; done
for compiler in 'cxx: g++' 'cxx: clang++'; do grep -F "$compiler" "$workflow" >/dev/null || fail "CI omits $compiler"; done
grep -F 'b_sanitize=address,undefined' "$workflow" >/dev/null || fail 'CI omits ASan+UBSan'
grep -F 'doxygen Doxyfile' "$workflow" >/dev/null || fail 'CI omits Doxygen'
for pin in \
  'repository: zeppe-lin/libpkgreconcile-apply' 'ref: v0.1.1' \
  'repository: zeppe-lin/libpkgapply-posix' 'ref: v3.2.1' \
  'repository: zeppe-lin/libpkgreconcile-posix'; do
  grep -F "$pin" "$workflow" >/dev/null || fail "CI omits pinned provider input: $pin"
done
for pin in \
  'libpkgsource, ref: v4.1.0' \
  'libpkgcatalog, ref: v4.0.0' \
  'libpkgresolve, ref: v4.0.0' \
  'libpkgbuild, ref: v3.0.1' \
  'libpkgbuild-image, ref: v1.0.1' \
  'libpkgsource-plan, ref: v2.0.0' \
  'libpkgbuild-plan, ref: v1.1.0' \
  'libpkgapply, ref: v3.0.1'
do
  grep -F "$pin" "$workflow" >/dev/null || fail "CI omits source-4 closure pin: $pin"
done
! grep -F 'libpkgsource, ref: v3.' "$workflow" >/dev/null || fail 'CI still admits source ABI 3'
printf '%s\n' 'ci-qualification-source-contract: ok'
