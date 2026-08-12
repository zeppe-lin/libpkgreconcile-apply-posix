# Maintaining

The maintainer invariant is simple: durable reconciliation truth is published
only after the concrete rejected object agrees with the completed semantic
projection.

Do not weaken the routing contract by inferring a rejected-store identity from a
filesystem path or descriptor. Do not replace direct identity reopening with a
directory scan. Do not retain reopened provider handles across the whole batch:
all-before-publish requires completed verification, not descriptor accumulation.
Do not move persistence into this layer.

Before release, qualify GCC and Clang, shared and static linkage, ASan+UBSan,
installed pkg-config consumers, Doxygen, the exact ELF ABI manifest, and every
named Meson suite. Dependency CI should pin published contracts rather than
floating repository heads.
