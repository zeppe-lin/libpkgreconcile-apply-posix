# Testing

Qualification is organized by authority, not by file count.

## Unit

The unit suite qualifies the stable `publication_error` model without requiring
provider state.

## Integration

The integration suite composes real planner/application evidence with the real
POSIX rejected-object and reconciliation stores. It covers:

- verified incoming evidence publication;
- idempotent republication;
- resolved-tombstone suppression;
- target-binding refusal;
- routed-store mismatch refusal;
- missing rejected-record refusal;
- reopened operation-plan mismatch refusal;
- reopened application-attempt mismatch refusal;
- reopened path mismatch refusal;
- reopened source-side mismatch refusal;
- all-before-publish atomicity for a mixed valid/invalid batch;
- bounded descriptor use for a large verified batch under a reduced
  `RLIMIT_NOFILE`; and
- verified old-object evidence becoming `prior_installed`.

The incoming fixture uses a symbolic-link object deliberately: the test is about
rejected-object verification and publication, not regular-payload staging.
Planner and application request use the same admitted archive identity.

## Header

Each installed public header is compiled independently, plus the umbrella
header. Public transitive requirements therefore remain part of the tested API.

## Contract

Source contracts enforce exact ABI, dependency closure, pkg-config metadata,
release metadata, documentation truth, Doxygen policy, CI qualification,
repository shape, style, and test topology.

There is no decorative `mechanism` suite: this repository owns composition, not
a new persistence or filesystem mechanism. There is no serialized protocol of
its own either.
