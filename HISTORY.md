# History

## 0.1.2

Application-generation-4 provider rebind.

- Require `libpkgreconcile-apply >= 0.1.2` and
  `libpkgapply-posix >= 4.0.0,<5.0.0`, so rejected-object verification is
  composed only with the owner-journal generation of the application provider.
- Qualify the shared product against `libpkgapply-posix.so.3` while preserving
  `libpkgreconcile-apply-posix.so.0`; the public publication symbol and provider
  object reference boundary are unchanged.
- Derive installed pkg-config requirements from the same Meson dependency
  objects used for compilation and linking, rejecting duplicate metadata.

## 0.1.1

Source-ABI-4 provider-closure release.

- Require `libpkgreconcile-apply >= 0.1.1` and `libpkgapply-posix >= 3.2.1`,
  excluding providers whose application closure could still select source ABI 3.
- Qualify the real publication composition against source/catalog/resolver 4,
  build 3.0.1, build-image 1.0.1, source-plan 2, and build-plan 1.1.
- Preserve `libpkgreconcile-apply-posix.so.0`; publication semantics and the
  reviewed public carrier ABI are unchanged.

## 0.1.0

Initial native provider-composition boundary.

- verify projected rejected locators against an explicitly routed
  `libpkgapply-posix` store;
- verify record identity, operation plan, physical application attempt, package
  path, and concrete source side;
- keep rejected-object verification descriptor use bounded independently of
  projection batch size;
- publish only a fully verified batch through `libpkgreconcile-posix`;
- preserve provider mechanism errors instead of translating them into generic
  composition failures; and
- qualify incoming and old rejected evidence with real provider integration.
