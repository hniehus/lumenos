# 7. Deployment View

## 7.1 Initial Deployment Target

```mermaid
flowchart TB
    Host[Developer Workstation] --> Toolchain[Compiler / Linker / Image Build]
    Toolchain --> BootImage[LumenOS Boot Image]
    BootImage --> QEMU[QEMU x86_64 VM]

    subgraph VM[QEMU Virtual Machine]
        CPU[x86_64 CPU]
        MEM[RAM]
        TMR[Timer Source]
        DEV[Minimal Virtual Devices]
        BL[Bootloader]
        KRN[LumenOS Kernel]
        USR[Initial User Space / Init]
    end

    QEMU --> CPU
    QEMU --> MEM
    QEMU --> TMR
    QEMU --> DEV
    QEMU --> BL
    BL --> KRN
    CPU --> KRN
    MEM --> KRN
    TMR --> KRN
    DEV --> KRN
    KRN --> USR
```

## 7.2 Deployment Decisions

- **x86_64 in QEMU first** — **Confirmed**
- The deployment path explicitly includes a **bootloader stage** — **Confirmed direction**
- Bare metal support is a later concern — **Confirmed direction**
- Additional architectures should only begin after the first userspace path is proven — **Working assumption**

## 7.3 Minimal Deployment Philosophy

The boot image must be sufficient to prove the end-to-end chain:

- bootloader starts
- kernel receives control through contract
- kernel reaches minimal internal readiness
- init starts
- further services remain optional for the first proof point

---
