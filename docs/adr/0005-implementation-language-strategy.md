# ADR 0005: Implementation Language Strategy

- **Status:** Accepted
- **Date:** 2026-04-16
- **Deciders:** LumenOS architecture team

## Context

LumenOS needs a durable implementation language strategy before the codebase grows large enough to fragment into multiple technical cultures.

Without an explicit language policy, low-level systems projects tend to drift into an unstable mix:
- core logic spreads across multiple languages,
- unsafe boundaries become hard to reason about,
- assembly use expands beyond the places where it is truly justified,
- and reviewability degrades because no one knows what the default should be.

The project already has a strong architectural direction:
- a narrow kernel boundary,
- explicit contracts,
- a capability-oriented object model,
- a minimal bring-up baseline on x86_64 + QEMU + Limine,
- and a preference for disciplined, reviewable progress.

The implementation language policy must reinforce that direction instead of undermining it.

## Decision

LumenOS adopts the following implementation language strategy:

### Primary language

**Rust is the default implementation language for LumenOS.**

Rust is the standard choice for:
- kernel logic above naked entry/exit stubs,
- kernel object handling,
- capability mediation,
- address-space and VMO logic,
- scheduler logic above the most CPU-near switching fragments,
- IPC logic,
- bootstrap logic above the lowest handoff layer,
- early user-space components and core services,
- tooling and test-related infrastructure where practical.

### Secondary language

**C23 is permitted only as a narrow interoperability and low-level boundary language.**

C23 may be used for:
- very small ABI shims,
- narrow compatibility layers,
- constrained low-level integration points where a C ABI is the clearest fit,
- sharply scoped cases where Rust is impractical for the specific boundary.

C23 is **not** a second default implementation language.

### Assembly policy

**Assembly is reserved for CPU-near code where the instruction set is effectively the interface.**

Assembly may be used for:
- boot entry fragments,
- trap and interrupt stubs,
- syscall entry/exit stubs,
- context-switch core fragments,
- early CPU mode/setup sequences,
- isolated architecture-specific instructions or sequences that cannot be expressed cleanly otherwise.

Use:
- **inline assembly** for small, local instruction sequences,
- **standalone assembly** for larger entry, trap, interrupt, or switching stubs.

## Normative rules

1. New implementation work **MUST** default to Rust unless a narrower language choice is justified.
2. C23 **MUST NOT** be used as a general-purpose fallback for ordinary kernel logic.
3. Assembly **MUST NOT** be used for logic that can be expressed clearly in Rust.
4. Assembly **SHOULD** be kept as small, isolated, and reviewable as possible.
5. Unsafe boundaries **MUST** be explicit and minimized.
6. Rust modules that rely on low-level unsafe or assembly-backed behavior **MUST** document the relevant assumptions and invariants.
7. Any expansion of C23 or assembly usage beyond the narrow policy in this ADR **MUST** be justified in a follow-up ADR or a documented architecture note tied to an accepted ADR.
8. The implementation language split **MUST** align with the kernel charter, boot contract, and bootstrap protocol.

## Practical language split

### Rust by default

Use Rust for:
- kernel object model implementation
- capability logic
- VM and mapping logic
- scheduler structure and policy
- IPC semantics and message-path logic
- bootstrap protocol handling
- init / root-task logic
- user-space runtime and services unless another language is explicitly justified

### C23 only for narrow shims

Use C23 only for:
- thin ABI surfaces
- tightly bounded compatibility code
- exceptional low-level glue where C is the least-worst boundary language

Do not use C23 for:
- core kernel subsystems
- policy logic
- the default implementation of new services
- broad bring-up logic

### Assembly only where machine state is the real contract

Use assembly for:
- `_start`-adjacent entry code
- trap/interrupt vectors and stubs
- syscall entry/exit stubs
- minimal context-switch core
- very early CPU setup transitions
- local instruction sequences that require register-exact control

Do not use assembly for:
- general control-flow-heavy logic
- subsystem orchestration
- policy logic
- convenience optimizations without evidence

## Rationale

This decision keeps the system coherent.

Rust carries the bulk of the implementation because it is the best fit for expressing invariants, ownership-sensitive behavior, and explicit interfaces in a disciplined codebase.

C23 remains available where interoperability or boundary conditions genuinely demand it, but it is intentionally constrained so that the project does not split into two primary implementation cultures.

Assembly remains available where the CPU itself is the contract, but it is kept narrow so the codebase does not become opaque and brittle.

This policy matches the broader architectural stance:
- small trusted core,
- explicit contracts,
- minimal hidden behavior,
- vertical progress before speculative breadth.

## Consequences

### Positive

- A clear default for new implementation work
- Smaller unsafe surface area in the long run
- Better reviewability
- Cleaner alignment between architecture and code
- Less risk of accidental C/assembly sprawl

### Negative

- Some low-level engineers may need to resist the temptation to write “just one more small thing” in C or assembly
- Early Rust low-level setup may require more discipline and care at FFI and assembly boundaries
- Some toolchain integration points may need small, explicit shims

## Rejected alternatives

### Treat Rust, C, and assembly as equal peers

Rejected because it creates ambiguity, inconsistent review expectations, and gradual architecture drift in the implementation layer.

### Use C23 as the main systems language and Rust only selectively

Rejected because it weakens the project’s ability to express invariants and contain unsafe behavior in the long run.

### Avoid assembly entirely

Rejected because some boot, trap, interrupt, and context-switch paths are inherently machine-contract code and should remain explicit.
