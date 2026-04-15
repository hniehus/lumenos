# 1. Introduction and Goals

## 1.1 Overview

LumenOS is being shaped as a **new operating system architecture**, not as a Linux clone with cosmetic changes. The current direction favors a **small, disciplined kernel core**, a strict boundary between kernel and user space, and a design that can scale from clean bring-up to a more capable and secure system over time.

The first milestone is brutally practical:

**boot cleanly on x86_64 in QEMU, execute a controlled boot handoff, establish the first kernel fastpath, and reach user-space initialization with architectural integrity still intact.**

The architecture is no longer defined only by the kernel internals. It is now also constrained by two explicit transition contracts:

- the **boot protocol and handoff contract** between bootloader and kernel
- the **bootstrap protocol** between the early kernel and the first user-space process

## 1.2 Primary Goals

- Build a kernel architecture that resists long-term erosion
- Keep the kernel small and intentional
- Make IPC, capability transfer, and isolation first-class concepts
- Optimize for a clean bring-up path before broader feature expansion
- Preserve a path toward stronger security and memory safety later without rewriting the whole system
- Define the boot path as a contract, not as an ad-hoc implementation accident

## 1.3 Top Quality Goals

1. **Architectural integrity** — no opportunistic growth of the kernel boundary
2. **Predictability** — early execution path must stay understandable and debuggable
3. **Isolation** — objects, address spaces, and communication paths are explicit
4. **Evolvability** — design choices today must not block later hardening
5. **Bring-up speed** — first platform and first fastpath take priority over premature generality
6. **Contract clarity** — every early-stage boundary must have defined responsibilities and handoff rules

---
