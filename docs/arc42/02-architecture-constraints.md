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

---
