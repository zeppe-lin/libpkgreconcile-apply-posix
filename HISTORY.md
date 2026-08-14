# History

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
