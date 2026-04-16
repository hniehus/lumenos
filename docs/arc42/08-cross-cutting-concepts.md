# 8. Cross-Cutting Concepts

## 8.1 Capability Model

Capabilities are the architectural glue between isolation and usability. They should become the standard way to reference authority over kernel objects.

Expected characteristics:

- explicit possession
- constrained delegation
- revocation model defined per object type
- no hidden ambient access path where avoidable

## 8.2 Object Ownership and Delegation

Ownership is not just bookkeeping. It defines failure domains, lifecycle, and transfer semantics. Every object must make ownership rules obvious.

## 8.3 Address Space and Memory Objects

The separation of **AddressSpace** and **VMO** suggests a design where memory-backed resources are distinct from mapping context. That is a strong base for later flexibility and security.

## 8.4 Scheduling and Time

Scheduling, timer integration, and IPC must be shaped as one interacting system during bring-up. The first implementation should resist policy overgrowth.

## 8.5 Device Access

The presence of **DeviceHandle** indicates a desire to avoid unrestricted device access patterns. Device-facing authority should remain explicit and mediated.

## 8.6 Boot and Bootstrap Contracts

The architecture now treats both early transitions as cross-cutting concerns:

- the **boot contract** prevents accidental assumptions at kernel entry
- the **bootstrap protocol** prevents vague or expanding responsibilities between early kernel code and init
- both contracts reduce ambiguity, make bring-up debuggable, and form a cleaner base for later security work

## 8.7 Implementation Language Strategy

The implementation language policy is a cross-cutting concept because it shapes how invariants, unsafe boundaries, and machine-near code are expressed across the entire system.

The architectural intent is simple:

- **Rust first**
- **C23 only at narrow boundaries**
- **Assembly only where the instruction set is effectively the interface**

### Why this matters architecturally

Language choice is not just a tooling preference. In LumenOS, it directly affects:

- how clearly invariants are expressed
- how much unsafe behavior is concentrated or dispersed
- how reviewable low-level code remains
- how visible CPU-contract code is
- how likely the system is to drift into accidental multi-language complexity

### Rust as the main implementation language

Rust carries the main body of the implementation because it is the best fit for:

- explicit interfaces
- ownership-sensitive logic
- kernel object handling
- capability mediation
- VM and bootstrap logic
- IPC logic
- service logic outside the kernel

The goal is not “Rust everywhere no matter what.”
The goal is to keep the default language aligned with architectural clarity and controlled unsafe boundaries.

### C23 as a constrained boundary language

C23 remains available, but only as a narrow tool for:
- ABI-oriented boundaries
- constrained interoperability layers
- small low-level shims where a C-shaped interface is the clearest boundary

The architectural risk of unconstrained C23 use is obvious:
the codebase begins to split into two default implementation cultures, and the system loses its disciplined center.

### Assembly as machine-contract code

Assembly is necessary in places where the machine itself is the interface.
Those places include:

- boot entry
- trap and interrupt stubs
- syscall entry and exit
- context-switch core
- very early CPU setup transitions
- tightly scoped instruction-level helpers

Architecture-wise, this code should remain:
- minimal
- isolated
- heavily constrained by comments and invariants
- easy to identify as special-purpose code

### Unsafe boundary strategy

Unsafe behavior should not spread invisibly across the codebase.

The preferred structure is:

- keep most logic in Rust
- isolate machine-near code into tight assembly or low-level boundary layers
- make Rust-to-assembly and Rust-to-C boundaries explicit
- document assumptions where correctness depends on calling convention, register state, memory layout, or privileged CPU behavior

This is a cross-cutting quality concern, not merely a code-style preference.

## 8.8 Security Direction

There is already a visible bias toward:

- explicit authority
- small trusted computing base
- strong isolation boundaries
- future hardening without architectural inversion

The exact hardening stack remains open.
