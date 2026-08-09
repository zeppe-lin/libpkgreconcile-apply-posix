# Contributing

Keep this repository a composition boundary.

Changes must not teach it rejected-store pathname grammar, reconciliation-store
serialization, package ownership semantics, controller sequencing, or store
discovery. New projection semantics belong in `libpkgreconcile-apply`; new
rejected-object mechanisms belong in `libpkgapply-posix`; new durable inventory
mechanisms belong in `libpkgreconcile-posix`.

Every semantic change needs unit or integration coverage at the owning level.
Run the unit, integration, header, and contract suites in both shared and static
builds. Shared ABI changes require an intentional update to
`abi/libpkgreconcile-apply-posix.exports` and release metadata.
