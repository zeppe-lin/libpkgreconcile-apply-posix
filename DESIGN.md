# Design

`libpkgreconcile-apply-posix` owns one composition rule: projected rejected
application evidence may become durable reconciliation state only after the
concrete POSIX rejected object has been reopened and shown to agree with the
projection.

## Authority chain

```text
completed application evidence
        |
        v
libpkgreconcile-apply
  semantic projection
        |
        v
orchestration routes rejected_object_store_identity
        |
        v
libpkgreconcile-apply-posix
  reopen + verify every tuple
        |
        v
libpkgreconcile-posix
  atomic pending publication
```

The generic adapter remains independent of provider mechanisms. The rejected
store remains an actuator-evidence mechanism. The reconciliation store remains
the persistence mechanism. This repository composes those authorities; it does
not replace them.

## Store routing is explicit

A rejected-object locator from `libpkgreconcile-apply` contains the exact
`rejected_object_store_identity` and `rejected_object_record_identity`.
Orchestration resolves the store identity to an authorized POSIX store handle.
This library receives both the routed identity and that handle.

There is intentionally no FD-to-store-identity derivation protocol here.
A filesystem directory does not become identity merely because it was opened.
The explicit identity is the routing assertion; the descriptor-anchored provider
handle is the mechanism authority. Their relationship is supplied by the routing
owner.

## Validation before publication

The complete batch is validated before durable publication. For each pending
value the composition layer requires:

- target binding equal to the reconciliation store binding;
- an adapter-owned and decodable rejected-object locator;
- locator store identity equal to the routed store identity;
- successful direct identity reopening through `libpkgapply-posix`;
- reopened record identity equal to the locator's record identity;
- reopened operation-plan identity equal to the projection's retained plan;
- reopened application-attempt identity equal to the projection's retained
  physical attempt;
- reopened observation path equal to the projected path; and
- reopened source equal to the projected retained side.

Each reopened rejected-object handle is released after its tuple has been fully
verified; all-before-publish does not require retaining provider handles for the
whole batch. Descriptor pressure is therefore bounded independently of the
number of pending tuples. Only after every tuple passes does the function invoke
one `inventory_generation_store::publish_pending()` operation. Verification
failure therefore cannot leave a prefix of the batch published.

## Negative boundary

This library does not:

- parse private rejected-store filenames;
- scan a rejected store for objects;
- derive store identity from directory paths or descriptors;
- reopen rejected bytes outside `libpkgapply-posix`;
- serialize or merge reconciliation inventories itself;
- inspect package ownership or installed state;
- choose keep/restore/delete/merge dispositions; or
- coordinate controller restart/publication ordering.

Those responsibilities remain with their existing owners.
