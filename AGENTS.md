# AGENTS.md

## Purpose

This file defines durable working rules for Codex in this repository.

It is intended to remain valid throughout the implementation of the system.
It should stay stable, practical, and short enough to remain trustworthy.

Use this file as the repository-wide default.
Add more specific `AGENTS.md` files in subdirectories only when local rules genuinely differ.

---

## Mission

Build the system with architectural discipline.

Optimize for:
- correctness before convenience
- explicit contracts before implicit behavior
- small, reviewable changes before large speculative rewrites
- vertical progress before breadth
- long-term maintainability over short-term cleverness

---

## Source of truth

When working in this repository, use the following order of authority:

1. Accepted ADRs in `docs/adr/`
2. Architecture documentation in `docs/arc42/`
3. Repository docs closest to the changed code
4. Existing code and tests
5. Task prompt and inline instructions

If sources conflict, prefer the more specific and more recently accepted architectural source.

Do not invent a new architecture in code when the docs already define one.

---

## Architectural guardrails

These rules are durable unless a later ADR changes them.

### Kernel boundary

- Treat the kernel boundary as intentional and narrow.
- Keep high-level policy out of the kernel.
- Prefer mechanism in kernel, policy in user space.
- Do not move functionality into the kernel only because it is faster to implement there.

### Object model

Respect the current kernel object model and its vocabulary.
Do not introduce new first-class kernel object types casually.
If a new object type is required, document the need and point to the ADR that authorizes it.

### Bring-up baseline

Until superseded by an ADR, the implementation baseline is:
- `x86_64`
- `QEMU`
- `Limine`

Do not dilute early implementation work with speculative portability.

### Boot and bootstrap contracts

Treat boot and bootstrap as explicit contracts, not as ad-hoc glue.

- The bootloader-to-kernel handoff must remain documented and testable.
- The kernel-to-init bootstrap path must remain minimal and explicit.
- Do not rely on undocumented machine state or accidental bootloader behavior.

---

## Default way of working

**BEFORE** you start, ask the user whether or not to create a branch, which branch (feature/fix/release) and propose a branch name.

**DO NOT** touch the build.counter

For every non-trivial task:

1. Understand the goal, constraints, and affected area.
2. Read the closest relevant docs before editing code.
3. Make the smallest change that moves the system forward.
4. Verify the change with the best available checks.
5. Update docs if the behavior, contract, or decision surface changed.

Prefer incremental progress over big-bang rewrites.

When a task is complex, ambiguous, or cross-cutting:
- make a short plan first
- call out assumptions explicitly
- identify the main risk before editing

---

## Change rules

### Allowed by default

- local refactors that improve clarity without changing architecture
- focused bug fixes
- tests that increase confidence
- small documentation updates that match implemented behavior
- plumbing required to complete the current vertical slice

### Not allowed by default

- architecture drift hidden inside implementation work
- silent changes to contracts, interfaces, or invariants
- broad rewrites without a clear payoff
- dependency additions without clear need
- mixing unrelated cleanup into the same change
- placeholder code presented as finished work

---

## Implementation rules

- Prefer simple, explicit code over clever abstractions.
- Keep functions and modules small enough to review comfortably.
- Preserve invariants and make them obvious in code.
- Add comments for contracts, invariants, and non-obvious reasoning.
- Do not add comments that merely narrate syntax.
- Keep naming consistent with the architecture docs and ADRs.
- Fail loudly on violated assumptions during bring-up rather than masking errors.
- Avoid hidden side effects in low-level code.
- Do not fake success paths.

---

## Implementation language policy

The implementation language strategy is part of the architecture and must remain stable unless changed by an ADR.

### Default language

- Default to **Rust** for new implementation work.
- Rust is the standard language for kernel logic, object handling, capability logic, VM logic, IPC logic, bootstrap logic, user-space services, and general system code.

### C23 usage

- Use **C23** only for narrow ABI, interoperability, or low-level shim layers.
- Do not use C23 as a second default language for ordinary kernel or service logic.
- Any new C23 area should be justified by a concrete boundary need.

### Assembly usage

- Use **inline assembly** only for small, local, architecture-specific instruction sequences.
- Use **standalone assembly** for boot entry, trap/interrupt stubs, syscall entry/exit stubs, context-switch core fragments, or other CPU-near code where the instruction set itself is the contract.
- Keep assembly minimal, isolated, and heavily constrained by comments and invariants.

### Unsafe boundary discipline

- Keep unsafe code small and explicit.
- Document assumptions at Rust/assembly and Rust/C boundaries.
- Do not spread low-level machine assumptions across unrelated modules.

### Escalation rule

If a change expands the role of C23 or assembly beyond the accepted language policy, stop and check the ADRs first.
If needed, create or update an ADR before proceeding.

---

## Testing and verification

Before considering a task done:

- run the most relevant tests, checks, or build commands available for the affected area
- if no automated test exists, perform the strongest practical verification and state it clearly
- for boot-path or kernel changes, prefer evidence from the real bring-up path over assumption
- do not claim tests passed if you did not run them
- do not ignore failing checks without explaining why

If the repo contains multiple test layers, prefer this order:
1. targeted checks for the changed area
2. integration or vertical-slice checks
3. broader validation when the change is cross-cutting

---

## Documentation rules

Update documentation when any of the following changes:
- architecture-relevant behavior
- public or internal contracts
- boot flow or bootstrap behavior
- kernel object semantics
- developer workflow required to build, test, or run the system

Create or update an ADR when a change:
- alters architecture
- introduces a new durable constraint
- changes the kernel boundary
- changes the object model
- changes the boot or bootstrap contract
- changes the bring-up target or baseline assumptions

Do not bury architectural decisions only in code or commit messages.

---

## Communication in task results

At the end of a task, report clearly:
- what changed
- why it changed
- how it was verified
- any remaining risks, assumptions, or follow-up work

Be honest about uncertainty.
Do not present guesses as facts.
Do not present incomplete work as production-ready.

---

## Repo hygiene

- Keep diffs focused.
- Avoid touching unrelated files.
- Preserve existing formatting conventions unless there is a good reason not to.
- Do not rewrite history inside the workspace unless explicitly asked.
- Do not delete useful documentation without replacing it.
- Prefer updating the nearest source of truth over creating duplicate docs.

---

## When in doubt

If a choice trades off speed against architectural integrity, choose integrity.

If a choice trades off abstraction against clarity, choose clarity.

If a choice trades off breadth against a proven vertical slice, choose the vertical slice.

If a change feels architectural, stop and check the ADRs before proceeding.
