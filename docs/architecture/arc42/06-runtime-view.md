# 6. Runtime View

## 6.1 Minimal Boot Path

The minimal boot path is now a first-class architectural artifact.

```mermaid
sequenceDiagram
    participant BL as Bootloader
    participant HC as Handoff Contract
    participant KE as Kernel Entry
    participant KB as Kernel Bootstrap
    participant MM as Minimal Memory Setup
    participant KC as Core Kernel Init
    participant INIT as Init Task / Thread
    participant US as Initial User Space

    BL->>HC: prepare agreed handoff state
    HC->>KE: transfer control
    KE->>KB: enter kernel bootstrap path
    KB->>MM: initialize minimal memory structures
    KB->>KC: initialize essential core subsystems only
    KC->>INIT: create initial task/thread/address space
    INIT->>US: start user-space bootstrap
```

## 6.2 Boot Protocol and Contract

The boot protocol defines the entry conditions under which the kernel may begin execution.

Architecturally, the contract exists to answer one brutal question clearly:

**what exactly may the kernel assume, and what must it still establish for itself?**

At the current level, the contract covers:

- who loads the kernel image
- who prepares the initial execution environment
- which bootstrap parameters or payload are handed over
- when ownership of execution passes to the kernel
- which assumptions are valid at kernel entry and which are not

## 6.3 Bootstrap Protocol Definition

Once inside the kernel, the second protocol begins. Its purpose is not to continue bootloader behavior but to define the controlled transfer from early kernel bootstrap to the first user-space process.

```mermaid
sequenceDiagram
    participant K as Kernel Bootstrap
    participant OBJ as Object Core
    participant VM as Address Space / VMO Setup
    participant SCH as Scheduler
    participant IPC as IPC Core
    participant I as Init Process

    K->>OBJ: create foundational kernel objects
    K->>VM: establish initial address space and memory objects
    K->>SCH: initialize scheduler core
    K->>IPC: initialize minimal communication path
    K->>I: create and populate init context
    SCH->>I: first dispatch to init
```

The bootstrap protocol is therefore responsible for:

- defining the kernel-side preconditions for init launch
- defining how init is identified and created
- defining what initial state or bootstrap data init receives
- ensuring that the transition to user space is minimal, deterministic, and inspectable

## 6.4 First Syscall / Fastpath Shape

```mermaid
sequenceDiagram
    participant U as User Thread
    participant T as Trap Entry
    participant S as Syscall Layer
    participant C as Capability Check
    participant I as IPC / Kernel Operation
    participant R as Return Path

    U->>T: syscall instruction
    T->>S: enter kernel with ABI-defined state
    S->>C: validate referenced capability / rights
    C->>I: dispatch allowed operation
    I->>R: produce result and next state
    R->>U: return to caller
```

## 6.5 Context Switch Focus

The context switch path is treated as one of the core architectural truth points. It must remain:

- minimal
- explicit in state ownership
- compatible with future isolation hardening
- measurable early

## 6.6 Synchronous IPC Focus

Synchronous IPC is part of the first fastpath, not a later add-on. This implies:

- IPC semantics influence the syscall ABI early
- capability handling must integrate with message endpoints from day one
- scheduling and IPC must be designed together, not sequentially

---
