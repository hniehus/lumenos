# ADR 0001: Kernel Charter

- **Status:** Accepted
- **Date:** 2026-04-16
- **Deciders:** LumenOS architecture team

## Context

LumenOS is intended to be a new operating system architecture with a small, disciplined kernel core and a strict separation between mechanism and policy.

Without an explicit kernel charter, the architecture will drift. The usual failure mode is predictable: once boot works and the first missing features hurt, more and more policy, convenience logic, and subsystem complexity leak into the kernel. That buys short-term progress and destroys long-term integrity.

The project therefore needs a normative boundary document that answers one question unambiguously:

**What belongs in the kernel, and what must stay out?**

## Decision

LumenOS adopts a **small-kernel charter**. The kernel is limited to a minimal set of mechanisms that are required to provide execution, isolation, object mediation, and the controlled transition into user space.

### In kernel

The following concerns are **in kernel** for the initial architecture baseline:

| Area | Included in kernel |
|---|---|
| Boot handoff consumption | Validation and consumption of the defined boot contract |
| Trap and exception handling | Low-level trap entry, exception dispatch, and return path |
| Syscall interface | Definition and implementation of the syscall ABI |
| Scheduling core | Runnable state transitions, dispatch, and minimal scheduling logic |
| Context switching | CPU state save/restore and address-space switch mechanics |
| IPC primitive | Minimal synchronous channel/endpoint-based IPC |
| Capability mediation | Capability validation, rights checks, and kernel object access control |
| Address-space core | Creation and management of address spaces and essential mappings |
| Memory object core | VMO lifecycle and mapping primitives required for execution |
| Interrupt core | Representation and controlled dispatch of interrupts |
| Timer core | Minimal timer objects and scheduler/timer integration |
| Device handle mediation | Minimal, explicit mediation of device-facing access where required |
| Bootstrap transfer to init | Creation of the first user-space context and handoff into init |

### Out of kernel

The following concerns are **out of kernel** unless a later ADR explicitly says otherwise:

| Area | Kept out of kernel |
|---|---|
| High-level policy | Resource arbitration policy, admission policy, placement policy |
| Service orchestration | Startup ordering and coordination of higher-level services |
| Rich resource management | Policy-rich memory, device, and service management |
| Filesystems | Filesystem implementations and storage policy |
| Driver logic | Most drivers, where technically feasible, run outside the kernel |
| Administrative tooling | Debug tooling, service tooling, and operational interfaces |
| Complex security policy | Policy engines and higher-level security coordination |
| Boot image tooling | Image assembly and build-time composition |
| User-space service frameworks | Init extensions, service managers, and application runtime layers |

## Normative rules

1. The kernel **MUST** provide mechanisms, not high-level policy.
2. Any new kernel feature **MUST** be justified by one of the following:
   - it is required for correctness of isolation or execution,
   - it is required for the minimal fastpath,
   - it cannot be implemented in user space without circular dependency or unacceptable architectural distortion.
3. Convenience alone is **NOT** a sufficient reason to move functionality into the kernel.
4. If a function can reasonably live in user space, it **SHOULD** live in user space.
5. The kernel **MUST NOT** become the default place for orchestration, policy accumulation, or feature spillover.
6. Any proposed exception to this charter **MUST** be documented in a follow-up ADR.

## Rationale

This decision preserves architectural integrity while the system is still young. It keeps the trusted computing base smaller, reduces long-term coupling, and protects the design from the common pattern where temporary shortcuts become permanent structure.

It also aligns with the existing emphasis on capabilities, IPC, address spaces, and explicit object mediation.

## Consequences

### Positive

- Cleaner long-term kernel boundary
- Smaller trusted computing base
- Stronger architectural pressure toward explicit contracts
- Better fit for capability-oriented and microkernel-like evolution
- Easier reasoning about what must be trusted

### Negative

- More early work in user-space bootstrap and service design
- Some features may take longer initially because kernel shortcuts are disallowed
- Driver and policy design must be more disciplined from the beginning

## Rejected alternatives

### Put more infrastructure into the kernel “for now”

Rejected because “for now” is how permanent kernel sprawl begins.

### Decide the boundary incrementally without a charter

Rejected because ambiguity is not neutral. It creates drift.
