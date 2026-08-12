# libpkgreconcile-apply-posix

`libpkgreconcile-apply-posix` is the native C++17 provider-composition boundary
between projected completed application evidence and durable POSIX
reconciliation state.

It joins three authorities without absorbing any of them:

- `libpkgreconcile-apply` says which completed rejected consequences should
  become package-independent pending reconciliation values;
- `libpkgapply-posix` reopens the exact immutable rejected object named by each
  projected locator; and
- `libpkgreconcile-posix` durably publishes the verified batch for the exact
  managed target.

The library does not plan application work, execute filesystem mutation, derive
rejected-store identity from a pathname or file descriptor, discover stores,
scan provider-private storage, choose reconciliation dispositions, or coordinate
transactions.

## Verified publication

`publish_verified_projection()` receives:

1. a completed `libpkgreconcile-apply` projection;
2. the exact `rejected_object_store_identity` that orchestration already routed;
3. an already-authorized `application_rejected_object_store` handle for that
   routed identity; and
4. an `inventory_generation_store` already bound to the projected managed
   target.

For every projected pending tuple it decodes the generic adapter's rejected
object locator and verifies:

- the locator names the routed rejected store;
- the exact rejected record reopens by identity;
- the reopened object reports that same record identity;
- the reopened record belongs to the projection's exact operation plan and
  physical application attempt;
- its observed package path equals the projected canonical path; and
- its concrete source (`incoming` or `old`) agrees with the projected
  reconciliation side (`incoming` or `prior_installed`).

Every tuple is verified before `publish_pending()` is called. Each reopened
provider object is released after that tuple is verified, so descriptor use
stays bounded independently of batch size. If any member of a batch fails
verification, no reconciliation generation is published by this operation.

## Routing boundary

`rejected_object_store_identity` is routing authority supplied by orchestration.
The POSIX rejected-store handle does not derive that identity from its directory
or descriptor, and this library does not claim otherwise. The caller is
responsible for resolving an exact store identity to an already-authorized store
handle. This library verifies that every projected locator names that routed
identity and verifies object-local facts through the provider.

Likewise, this library does not interpret the locator's opaque 64-byte payload
itself. It uses `libpkgreconcile-apply`'s public decoder. No rejected-store
pathname grammar enters this boundary.

## Failure model

Semantic refusals are reported as `publication_error` with a stable
`publication_error_code`. Provider mechanism failures remain owned by their
providers: `pkgapply::posix::rejected_store_error` and
`pkgreconcile::posix::store_error` propagate unchanged.

## Dependencies

The product dependency boundary is exactly:

- `libpkgreconcile-apply >= 0.1.0`;
- `libpkgapply-posix >= 3.2.0`;
- `libpkgreconcile-posix >= 0.1.0`.

Planner/build libraries used by the integration fixture are qualification-only.

## Building

```sh
meson setup build
meson compile -C build
meson test -C build --print-errorlogs
```

Static builds use matching `default_library=static` and `link_mode=static`.
