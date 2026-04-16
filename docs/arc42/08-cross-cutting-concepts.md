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

## 8.7 Security Direction

There is already a visible bias toward:

- explicit authority
- small trusted computing base
- strong isolation boundaries
- future hardening without architectural inversion

The exact hardening stack remains open.

---
