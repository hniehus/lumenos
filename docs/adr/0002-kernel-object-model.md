# ADR 0002: Kernel Object Model

- **Status:** Accepted
- **Date:** 2026-04-16
- **Deciders:** LumenOS architecture team

## Context

A capability-oriented kernel cannot remain conceptually stable if its objects are vague. The system needs a minimal and explicit kernel object model early, before convenience cases and subsystem growth introduce hidden exceptions.

The object model must answer:

- Which kernel objects exist in the first architecture baseline?
- What does each object represent?
- What are the required lifecycle concerns for each object?
- How are ownership, delegation, and revocation treated?

## Decision

LumenOS adopts the following **initial kernel object set**:

1. **Task / Process**
2. **Thread**
3. **AddressSpace**
4. **VMO**
5. **Capability**
6. **Channel / Endpoint**
7. **Interrupt**
8. **Timer**
9. **DeviceHandle**

These are the minimal kernel objects for the first architecture baseline.

## Object definitions

| Object | Purpose |
|---|---|
| Task / Process | Ownership and execution container; principal boundary for execution context and object association |
| Thread | Schedulable execution unit |
| AddressSpace | Virtual memory context used for mappings and execution |
| VMO | Virtual memory object that backs mappings, sharing, or executable image placement |
| Capability | Explicit authority to reference and operate on a kernel object |
| Channel / Endpoint | IPC primitive for controlled communication |
| Interrupt | Kernel representation of an interrupt source or interrupt binding |
| Timer | Kernel representation of time-based wakeups or timed events |
| DeviceHandle | Controlled representation of device-facing access |

## Normative model rules

1. Every kernel object **MUST** have a defined lifecycle.
2. Every kernel object **MUST** have an ownership model.
3. Any access to a kernel object **MUST** be mediated through an explicit capability or an equivalent kernel-internal authority path.
4. Ambient, implicit, or global authority **MUST NOT** be the default access model.
5. Delegation semantics **MUST** be defined before broader subsystem growth builds on top of the object.
6. Revocation semantics **MUST** be considered part of the object contract, even if the first implementation is minimal.
7. The set of kernel objects **MUST NOT** grow casually; new object types require an ADR.
8. Capability rights **MUST** be represented as explicit bits, and delegation **MUST NOT** increase those rights.
9. Invalid capabilities, wrong object types, and insufficient rights **MUST** fail deterministically.

## Required lifecycle concerns per object

For each object type, the following concerns are mandatory:

- **Creation** — who may create it, and under which prerequisites
- **Ownership** — who owns it, manages it, or is accountable for its lifecycle
- **Delegation** — whether and how authority may be transferred
- **Revocation** — how authority may be withdrawn or invalidated
- **Destruction** — how the object is torn down and what dependencies must be resolved first

## Initial object relationships

The following relationships define the first architectural shape:

- A **Task / Process** contains or owns one or more **Threads**
- A **Thread** executes within an **AddressSpace**
- An **AddressSpace** contains mappings to one or more **VMOs**
- A **Capability** references an object and constrains allowed operations on it
- A **Channel / Endpoint** enables controlled IPC between principals
- An **Interrupt** and **Timer** are first-class kernel event objects, not hidden side paths
- A **DeviceHandle** represents controlled device-facing authority rather than unrestricted access

## Capability semantics

Capabilities are valid only when all of the following remain true:

- the capability itself is valid
- the referenced object type matches the requested operation
- the referenced object identity matches the target of the operation
- the granted rights mask contains every bit required for the operation

Delegation is an attenuation step. A delegated capability may preserve or reduce rights, but it may not add rights that were not already present in the parent capability.

## Initial invariants

The initial kernel object model establishes the following invariants:

1. No kernel object is “special” in a way that bypasses the authority model without explicit justification.
2. Object identity and authority are separate concerns.
3. Execution, memory, communication, time, and device interaction are represented explicitly.
4. The kernel object model favors a small number of orthogonal primitives over a large number of convenience abstractions.

## Out of scope for this ADR

The following are intentionally **not** introduced as first-class kernel objects in this ADR:

- filesystem objects
- sockets or rich networking objects
- high-level service descriptors
- policy objects
- application framework objects

Those may be introduced later only if justified by architecture and documented in ADRs.

## Rationale

This model is small enough to keep the design coherent and large enough to support the first bring-up path into user space. It matches the current emphasis on explicit authority, IPC, isolation, and controlled device access.

It also creates a disciplined foundation for later protocol decisions such as the bootstrap contract and syscall ABI.

## Consequences

### Positive

- Shared vocabulary for the kernel design
- Cleaner lifecycle reasoning
- Better fit for capability-oriented access control
- Easier ADR work for syscall, IPC, and bootstrap design

### Negative

- More up-front design discipline is required
- Some implementation details must be deferred until lifecycle and delegation semantics are better specified

## Rejected alternatives

### Start implementation first and let the object model emerge later

Rejected because emergent object models tend to encode accidents, not architecture.

### Introduce many specialized object types immediately

Rejected because it increases surface area before the core relationships are proven.
