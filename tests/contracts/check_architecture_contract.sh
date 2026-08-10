#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
fail() { echo "architecture-contract: $*" >&2; exit 1; }

# Product code may compose exactly the generic adapter and the two POSIX
# providers. It must not absorb controller, state, planner, or raw POSIX layout
# authority.
if grep -RInE '#[[:space:]]*include[[:space:]]*<(libpkgstate|pkgctl|libpkgtransaction|libpkgplan|unistd\.h|fcntl\.h|sys/)' \
    "$root/include" "$root/src" --include='*.h' --include='*.cpp' >/dev/null; then
  fail 'product imports controller, state, planner, transaction, or raw POSIX authority'
fi

actual=$(perl -0777 -ne '
  while (/dependency\s*\(\s*['\''"]([^'\''"]+)['\''"]/g) {
    print "$1\n"
  }
' "$root/meson.build" | sort -u)
expected='libpkgapply-posix
libpkgreconcile-apply
libpkgreconcile-posix'
[ "$actual" = "$expected" ] || {
  printf '%s\n' "$actual" >&2
  fail 'product dependency boundary is not exactly the three composition owners'
}

if grep -RInE '/var/lib/pkg/rejected|by-id-v[0-9]|record-v[0-9]-|generations/|renameat2|flock\(' \
    "$root/include" "$root/src" >/dev/null; then
  fail 'product contains provider-private storage/layout grammar'
fi

printf '%s\n' 'architecture-contract: ok'
