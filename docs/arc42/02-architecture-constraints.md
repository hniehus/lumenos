# 2. Architecture Constraints

## 2.1 Technical Constraints

- **Start platform: x86_64 under QEMU** — **Confirmed**
- Early focus is on a **single target platform** to avoid fragmentation during bring-up — **Confirmed**
- The first fastpath must cover the following elements — **Confirmed**
  - trap entry
  - syscall ABI
  - context switch
  - synchronous IPC path
  - minimal scheduler tick / timer integration
- The boot path must follow a **formal boot contract** rather than an implicit handoff — **Confirmed**
- The kernel must accept control only after a **defined handoff state** has been established by the bootloader — **Confirmed direction**
- The bootstrap path from kernel to first user space must follow a **formal bootstrap protocol** — **Confirmed**
- The first boot path must remain **minimal by design**, with only essential steps and components enabled — **Confirmed**

## 2.2 Organizational / Design Constraints

- The architecture must be documented early enough to prevent kernel-boundary drift — **Confirmed direction**
- A **Kernel Charter** must define what belongs **in kernel** and **out of kernel** — **Confirmed direction**
- The kernel object model must be explicit from the beginning — **Confirmed direction**
- The first design must prefer **clarity over breadth** and **validated foundations over feature ambition** — **Confirmed direction**
- Bootloader responsibility, kernel responsibility, and init responsibility must not blur into one another — **Confirmed direction**

## 2.3 Working Architectural Assumption

Based on the current object model and the emphasis on channels, capabilities, address spaces, and IPC, the most coherent interpretation is:

- **LumenOS is currently being shaped as a capability-oriented microkernel or near-microkernel design** — **Working assumption**

This should be formally ratified in an ADR once the kernel type decision is frozen.

## 2.4 Boot and Bootstrap Contract Assumptions

The currently visible interpretation of the decisions from boot protocol, bootstrap protocol, and minimal boot path is:

- the **bootloader** is responsible for loading the kernel image, preparing the minimal execution environment, and performing the handoff according to contract
- the **kernel** is responsible for validating and consuming that handoff, initializing only essential core subsystems, and establishing the first runnable user-space context
- the **init process** is responsible for the first higher-level bootstrap actions after kernel handoff is complete

These assumptions are now treated as part of the architecture baseline and should be formalized in ADRs and protocol specifications.

## 2.5 Implementation Language Constraint

The implementation language strategy is part of the architecture, not a local coding preference.

For the current baseline, LumenOS adopts the following language constraint:

- **Rust is the default implementation language**
- **C23 is restricted to narrow low-level or interoperability boundaries**
- **Inline and standalone assembly are restricted to CPU-near paths where machine state or instruction-level control is the actual contract**

This constraint exists to preserve architectural coherence as the system grows.

### Language roles

#### Rust

Rust is the default language for:
- kernel logic above the most CPU-near entry and exit fragments
- kernel object handling
- capability mediation
- address-space and VMO logic
- scheduler structure and most scheduling logic
- IPC logic
- bootstrap handling above the lowest handoff layer
- user-space runtime and service code by default

#### C23

C23 is allowed only for:
- thin ABI shims
- sharply bounded low-level compatibility layers
- constrained interoperability boundaries where a C ABI is the clearest fit

C23 is **not** intended to become a second general-purpose systems language for LumenOS.

#### Inline or standalone assembly

Assembly is allowed for:
- early boot entry fragments
- trap and interrupt stubs
- syscall entry and exit stubs
- context-switch core fragments
- very early CPU setup transitions
- isolated architecture-specific instruction sequences requiring exact control

Assembly is not the default expression language for general kernel behavior.

### Constraint rationale

This language split supports the broader architectural goals:

- keep the main body of the system expressive and reviewable
- keep unsafe boundaries small and intentional
- keep machine-contract code visible and isolated
- prevent accidental drift into a mixed-language kernel without clear rules

### Constraint consequence

Any intentional expansion of C23 or assembly beyond this narrow role should be treated as an architecture-affecting change and evaluated through an ADR.
