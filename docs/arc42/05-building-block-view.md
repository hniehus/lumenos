# 5. Building Block View

## 5.1 Whitebox Overview

```mermaid
flowchart TB
    subgraph BootDomain[Boot Domain]
        BL[Bootloader]
        HC[Boot Handoff Contract]
        BP[Bootstrap Payload / Parameters]
    end

    subgraph Kernel[Kernel Space]
        Trap[Trap / Interrupt Entry]
        Syscall[Syscall Interface]
        Sched[Scheduler]
        IPC[Synchronous IPC Path]
        Cap[Capability System]
        Obj[Kernel Object Management]
        VM[Address Space / VMO Management]
        Time[Timer / Tick Integration]
        Intr[Interrupt Handling]
        DevH[Device Handle Layer]
        KBoot[Kernel Bootstrap Logic]
    end

    subgraph User[User Space]
        Init[Init Process]
        RM[Resource / Runtime Services]
        SM[Service Managers]
        Drivers[User-Space Drivers / Services]
        Apps[Applications]
    end

    BL --> HC
    HC --> KBoot
    BP --> KBoot
    KBoot --> Obj
    KBoot --> VM
    KBoot --> Sched
    KBoot --> IPC
    Trap --> Syscall
    Syscall --> Cap
    Syscall --> IPC
    Syscall --> VM
    Syscall --> Obj
    Time --> Sched
    Intr --> IPC
    KBoot --> Init
    IPC --> Init
    Cap --> RM
    VM --> Init
    DevH --> Drivers
```

## 5.2 Initial Kernel Object Model

The current minimal kernel object set is:

- **Task / Process** — execution container and ownership boundary
- **Thread** — schedulable execution unit
- **AddressSpace** — virtual memory context
- **VMO** — virtual memory object / memory-backed mapping primitive
- **Capability** — explicit right to reference and operate on kernel objects
- **Channel / Endpoint** — IPC primitive
- **Interrupt** — interrupt representation / binding target
- **Timer** — timer object for scheduling and timed events
- **DeviceHandle** — controlled access path to device-facing functionality

## 5.3 Object Lifecycle Concerns

Each object type must define these aspects explicitly:

- creation
- ownership
- delegation
- revocation
- destruction / lifecycle end

This is not documentation garnish. It is part of the architecture itself.

## 5.4 Boot-Related Responsibilities

The minimal boot path sharpens the responsibility split:

### Bootloader responsibilities

- load the kernel image
- prepare the initial execution environment
- provide the agreed bootstrap parameters / payload
- transfer control only through the defined contract

### Kernel bootstrap responsibilities

- consume and validate handoff state
- initialize only essential memory and core execution structures
- establish the first kernel objects needed for bootstrap
- create the first user-space task/thread/address-space combination
- transfer control to init according to bootstrap protocol

### Init responsibilities

- receive bootstrap state from the kernel-defined interface
- perform the first higher-level user-space initialization steps
- bring up the next layer of services outside the kernel

## 5.5 Inside vs Outside the Kernel

The final Kernel Charter is still to be written, but the current direction is already visible.

```mermaid
flowchart LR
    subgraph InKernel[In Kernel - current direction]
        IK1[Boot handoff consumption]
        IK2[Trap handling]
        IK3[Syscall ABI]
        IK4[Scheduling core]
        IK5[IPC primitive]
        IK6[Capability checks on kernel objects]
        IK7[Address-space core]
        IK8[Interrupt / timer core]
        IK9[Minimal device handle mediation]
        IK10[Bootstrap transfer to init]
    end

    subgraph OutKernel[Out of Kernel - current direction]
        OK0[Boot image construction tooling]
        OK1[Higher-level services]
        OK2[Policy-rich resource managers]
        OK3[Most drivers where feasible]
        OK4[User-space service orchestration]
        OK5[Filesystem and richer subsystems]
        OK6[Administrative tooling]
    end
```

---
