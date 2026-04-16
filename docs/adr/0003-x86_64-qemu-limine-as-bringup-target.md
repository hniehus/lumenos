# ADR 0003: x86_64 + QEMU + Limine as Bring-Up Target

- **Status:** Accepted
- **Date:** 2026-04-16
- **Deciders:** LumenOS architecture team

## Context

A new operating system cannot validate its architecture on abstractions alone. It needs a concrete bring-up target with a repeatable execution environment, deterministic debugging conditions, and minimal friction for kernel iteration.

If the project tries to stay “portable from day one,” it will fragment its attention before the first end-to-end proof exists.

The project therefore needs to fix the first target platform and boot path early.

## Decision

LumenOS selects the following **initial bring-up target**:

- **Architecture:** x86_64
- **Execution environment:** QEMU
- **Bootloader / boot protocol baseline:** Limine

This is the **only official bring-up target** until the first end-to-end path from bootloader to user space is proven and repeatable.

## Scope of this decision

This decision fixes the initial execution baseline for the project. It does **not** claim that x86_64, QEMU, or Limine are permanent forever. It claims that they are the correct first proving ground.

The first proving ground must support fast iteration on:

- early boot
- trap entry
- syscall ABI
- context switch
- synchronous IPC
- minimal timer / scheduler integration

## Normative rules

1. All early bring-up work **MUST** target x86_64 on QEMU first.
2. Limine is the initial bootloader baseline and **MUST** be used for the first boot contract.
3. Additional architectures or bootloader stacks **MUST NOT** distract from the first proven vertical slice.
4. Bare-metal ambitions **MUST** remain secondary until the QEMU-based bring-up path is stable and reproducible.
5. Any expansion beyond this baseline requires a follow-up ADR.

## Rationale

This decision optimizes for learning velocity and architectural validation.

- **x86_64** is well understood and practical for early kernel work.
- **QEMU** provides a controllable environment for repeatable bring-up and debugging.
- **Limine** offers a modern and pragmatic boot baseline for the chosen environment.

Together, they create the shortest credible path to proving the architecture rather than merely discussing it.

## Consequences

### Positive

- Faster bring-up iterations
- Better reproducibility
- Lower early complexity
- Clear target for boot contract and bootstrap work
- Less ambiguity in toolchain and debugging setup

### Negative

- Early work is intentionally platform-specific
- Portability is delayed by design
- Some later redesign may be required when adding more targets

## Rejected alternatives

### Support multiple architectures from the beginning

Rejected because it optimizes for breadth before truth.

### Start on bare metal first

Rejected because it increases bring-up friction without improving the first architectural proof.

### Defer bootloader choice

Rejected because a fuzzy boot path blocks a precise boot contract.
