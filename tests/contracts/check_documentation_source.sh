#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
fail() { echo "documentation-source-contract: $*" >&2; exit 1; }

for required in README.md DESIGN.md HISTORY.md CONTRIBUTING.md MAINTAINING.md TESTING.md man/libpkgreconcile-apply-posix.3.scdoc; do
  [ -s "$root/$required" ] || fail "missing or empty $required"
done
for document in README.md DESIGN.md HISTORY.md CONTRIBUTING.md MAINTAINING.md TESTING.md; do
  case $(sed -n '1p' "$root/$document") in '# '*) ;; *) fail "$document does not start with an ATX level-one heading" ;; esac
done

for document in "$root/README.md" "$root/DESIGN.md" "$root/man/libpkgreconcile-apply-posix.3.scdoc"; do
  grep -F 'routed' "$document" >/dev/null || fail "$(basename "$document") omits explicit store routing"
  grep -F 'rejected_object_store_identity' "$document" >/dev/null || fail "$(basename "$document") omits routed store identity"
done

grep -F 'Every tuple is verified before' "$root/README.md" >/dev/null || fail 'README omits all-before-publish guarantee'
grep -F 'does not derive' "$root/README.md" >/dev/null || fail 'README invents or omits store-identity derivation boundary'
grep -F 'There is intentionally no FD-to-store-identity derivation protocol' "$root/DESIGN.md" >/dev/null || fail 'DESIGN omits explicit routing limitation'
grep -F 'Only after every tuple passes' "$root/DESIGN.md" >/dev/null || fail 'DESIGN omits verification-before-publication order'
grep -F 'Descriptor pressure is therefore bounded independently' "$root/DESIGN.md" >/dev/null || fail 'DESIGN omits bounded descriptor verification'
grep -F 'does not derive that' "$root/man/libpkgreconcile-apply-posix.3.scdoc" >/dev/null || fail 'manual omits routing limitation'
for document in "$root/README.md" "$root/DESIGN.md" "$root/man/libpkgreconcile-apply-posix.3.scdoc"; do
  grep -Ei 'operation[- ]plan' "$document" >/dev/null || fail "$(basename "$document") omits plan binding verification"
  grep -Ei 'application[- ]attempt' "$document" >/dev/null || fail "$(basename "$document") omits attempt binding verification"
done

if grep -RInE '/var/lib/pkg/rejected|by-id-v[0-9]|record-v[0-9]-|generations/|renameat2|flock\(' \
    "$root/README.md" "$root/DESIGN.md" "$root/TESTING.md" "$root/MAINTAINING.md" "$root/man" >/dev/null; then
  fail 'provider-private storage grammar leaked into public documentation'
fi

printf '%s\n' 'documentation-source-contract: ok'
