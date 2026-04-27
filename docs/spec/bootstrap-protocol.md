# LumenOS Bootstrap Protocol
Status: Draft  
Applies to: x86_64 bring-up under QEMU, Limine-based boot  
Scope: Kernel -> first userspace task handoff

## 1. Purpose

This document defines the initial bootstrap contract between the LumenOS kernel and the first userspace task.

The goal is to make the first userspace transition:

- minimal
- explicit
- capability-bounded
- implementable in the first bring-up
- extensible without breaking the ABI immediately

The first userspace task does **not** receive broad, implicit system authority.  
It receives only the objects and rights required to establish the next system stage.

---

## 2. Decision Summary

### 2.1 First userspace task

The first userspace process is a **minimal bootstrap task**.

Canonical name:

- `bootstrap-task`

Expected binary name in bring-up:

- `bootstrap.elf`

### 2.2 Why this choice

LumenOS does **not** start directly with a classic all-powerful `init`.
It also does **not** start with a pure `sigma0`-style pager only.

Instead, the first task is a narrowly scoped bootstrap process whose job is to:

- read boot information
- take ownership of the initial boot memory capabilities
- establish the first userspace runtime foundations
- create and start the next long-lived system tasks
- hand off control and reduce its own relevance

This keeps early bring-up simple while preserving the architectural direction toward a capability-based microkernel.

---

## 3. Responsibilities of the Bootstrap Task

The bootstrap task is responsible for:

1. validating the BootInfo structure
2. establishing early userspace memory management from granted boot memory objects
3. loading or mapping the next userspace binaries
4. creating the first child tasks/threads
5. starting the long-lived `init` task
6. optionally starting a dedicated pager/resource manager later
7. terminating or becoming inactive after handoff

The bootstrap task is **not** a permanent privileged super-process by architectural principle.

---

## 4. Objects Created by the Kernel Initially

At the kernel -> userspace handoff, the kernel creates exactly the following initial objects.

### 4.1 Mandatory objects

- first `Task`
- first `Thread`
- first `AddressSpace`
- read-only `BootInfo` mapping
- one or more boot memory `VMO`s representing controlled usable memory
- optional read-only module `VMO`s for boot modules

### 4.2 Deliberately not pre-created

- no global interrupt-management object
- no device-management object by default
- no privileged kernel control channel
- no arbitrary pre-created IPC channel pair
- no implicit authority over all physical memory
- no authority over other tasks or future global services

### 4.3 IPC decision

**Initial general-purpose IPC objects: none.**

Reason:

The bootstrap task shall create the first channel explicitly through normal kernel object creation once it is running.
This avoids hidden privileged backchannels between kernel and userspace.

---

## 5. Initial Address Space Layout

The kernel creates the first userspace address space and maps only the minimum required regions.

### 5.1 Required mappings

1. `bootstrap.elf` load segments  
   - executable segments mapped RX
   - read-only data mapped R
   - writable data/BSS mapped RW
   - writable+executable user pages are invalid

2. initial user stack  
   - mapped RW
   - at least one guard page
   - initial size implementation-defined for bring-up
   - recommended initial size: 64 KiB
   - the guard page remains unmapped and must fault on access

3. BootInfo mapping  
   - mapped read-only
   - present at entry
   - virtual address passed in a register at task entry

### 5.2 Mapping rules

The kernel must expose explicit mapping classes rather than a generic ambient mapping model.

- kernel-only mappings are never tagged user-accessible
- user mappings are created as read-only, read-write, or executable
- user read-write mappings must not also be executable
- user access to kernel-only pages must fault
- the initial user stack is a bounded mapping with an unmapped guard page
- BootInfo remains read-only for the first task

### 5.3 Optional mappings

- optional read-only mappings for boot modules are allowed
- default preference: boot modules are passed as `VMO` capabilities and mapped by userspace explicitly

### 5.4 Hard rules

- kernel memory is not mapped into userspace
- no direct identity map of physical memory is exposed to the first task
- no writable mapping of BootInfo is allowed
- no MMIO region is mapped by default

---

## 6. Entry Convention

The first userspace thread starts in ring 3 / user mode at the ELF entry point of `bootstrap.elf`.

### 6.1 Register contract on entry (x86_64)

- `RDI` = virtual address of `BootInfo`
- `RSP` = top of initial user stack
- `RIP` = ELF entry point of `bootstrap.elf`

All other general-purpose registers are unspecified unless defined later.

### 6.2 Processor state expectations

At userspace entry:

- CPU is in long mode
- userspace runs at privilege level 3
- paging is active
- direction flag must be clear
- user-visible segment state must be valid for normal 64-bit userspace execution
- interrupts follow normal post-sysret/iret userspace semantics as defined by the kernel implementation

---

## 7. BootInfo Format

BootInfo is the authoritative, read-only kernel -> userspace handoff structure.

It contains:

- ABI/version information
- architecture information
- boot memory map
- module information
- capability descriptor table
- optional platform discovery pointers passed through from boot

BootInfo is mapped read-only into the bootstrap task.

### 7.1 BootInfo top-level structure

```c
struct lumen_boot_info {
    uint64_t magic;                 // 'LMBOOTI\0' or equivalent constant
    uint32_t abi_major;
    uint32_t abi_minor;
    uint32_t total_size;
    uint32_t flags;

    uint64_t page_size;
    uint64_t bootinfo_vaddr;

    uint64_t memory_map_offset;
    uint32_t memory_map_count;
    uint32_t memory_map_entry_size;

    uint64_t module_offset;
    uint32_t module_count;
    uint32_t module_entry_size;

    uint64_t cap_offset;
    uint32_t cap_count;
    uint32_t cap_entry_size;

    uint64_t rsdp_phys;             // 0 if unavailable
    uint64_t framebuffer_info_off;  // 0 if unavailable / unused
    uint64_t reserved0;
};
```

### 7.2 Memory map entry

```c
struct lumen_boot_mem_entry {
    uint64_t base_phys;
    uint64_t length;
    uint32_t type;      // usable, reserved, kernel, bootloader, acpi, mmio, etc.
    uint32_t flags;
};
```

The memory map is informational.
It does **not** itself grant authority.

### 7.3 Module entry

```c
struct lumen_boot_module_entry {
    uint32_t id;
    uint32_t flags;
    uint64_t name_offset;      // string inside BootInfo blob
    uint64_t cmdline_offset;   // string inside BootInfo blob, optional
    uint32_t cap_slot;         // slot of module VMO capability
    uint32_t reserved0;
};
```

### 7.4 Capability descriptor entry

```c
struct lumen_boot_cap_entry {
    uint32_t slot;
    uint16_t object_type;
    uint16_t flags;
    uint64_t rights_mask;
    uint64_t purpose;          // semantic role enum
};
```

The capability descriptor table documents what the kernel has already installed into the initial handle table.

---

## 8. Initial Capability Model

The first task starts with an already populated handle table.
Handles are identified by slot numbers.

### 8.1 Slot rules

- slot `0` is always invalid
- slots `1..15` are reserved for mandatory bootstrap capabilities
- slots `16..63` are reserved for boot memory capabilities
- slots `64..127` are reserved for boot module capabilities
- higher slots are reserved for future protocol extensions

### 8.2 Mandatory capability slots

| Slot | Name | Object Type | Rights | Purpose |
|------|------|-------------|--------|---------|
| 1 | `TASK_SELF` | Task | inspect, create_child_task, create_thread, create_channel, duplicate_owned_caps | bootstrap task self-reference |
| 2 | `THREAD_SELF` | Thread | inspect, exit_self | current thread self-reference |
| 3 | `ADDRESS_SPACE_SELF` | AddressSpace | inspect, map, unmap, protect | manage own address space |
| 4 | `BOOTINFO_VMO` | VMO | map_readonly, duplicate_readonly | optional re-map/share of BootInfo |

### 8.3 Boot memory capabilities

Slots `16..63` may contain one or more `BOOT_MEMORY_VMO` capabilities.

Properties:

- each represents a controlled memory object seeded by the kernel
- together they form the initial userspace memory authority
- they are the basis for early allocator/pager/resource-manager setup

Allowed rights:

- map
- read
- write
- derive / split
- transfer / duplicate

Not allowed by default:

- implicit device/MMIO mapping authority
- implicit interrupt association
- implicit DMA authority

### 8.4 Boot module capabilities

Slots `64..127` may contain read-only `MODULE_VMO` capabilities.

Typical use:

- `init.elf`
- future pager binary
- initramfs / bootstrap archive
- test binaries during bring-up

Allowed rights:

- read
- map_readonly
- duplicate_readonly

---

## 9. Explicitly Excluded Rights

The bootstrap task does **not** start with the following rights.

### 9.1 No interrupt control

The first task receives:

- no interrupt object
- no IRQ routing authority
- no APIC / IOAPIC control authority
- no system-wide timer-interrupt management authority

### 9.2 No device authority by default

The first task receives:

- no generic `DeviceHandle`
- no raw MMIO mapping authority
- no I/O port authority
- no PCI enumeration authority unless added explicitly in a later protocol revision

### 9.3 No global memory omnipotence

The first task does **not** receive:

- an implicit right to all free physical frames
- arbitrary physical mapping rights
- access to reserved/kernel/bootloader memory regions just because they are listed in BootInfo

Only explicitly granted boot memory VMOs constitute authority.

### 9.4 No hidden kernel control plane

The first task receives:

- no special privileged kernel backchannel
- no secret control socket/channel
- no “god mode” debug capability by default

### 9.5 No authority over unrelated future tasks

The bootstrap task does not automatically gain authority over all later tasks.
Authority must be propagated explicitly through capability transfer.

---

## 10. Kernel Loading Rule for the First Userspace Task

For the bring-up phase, the kernel expects one required boot module containing `bootstrap.elf`.

Recommended source:

- loaded by Limine as a boot module

Kernel responsibilities:

1. locate the bootstrap module
2. validate ELF format minimally
3. create first address space
4. map ELF segments according to flags
5. create initial stack
6. create BootInfo
7. install initial capabilities
8. enter userspace

Failure to locate or load `bootstrap.elf` is a fatal boot error.

---

## 11. Expected Bootstrap Flow

The intended first-stage flow is:

1. kernel enters userspace at `bootstrap.elf`
2. bootstrap task validates BootInfo ABI and required slots
3. bootstrap task establishes early allocator/pager logic from `BOOT_MEMORY_VMO`s
4. bootstrap task loads or maps the next binary or binaries
5. bootstrap task creates child task(s)
6. bootstrap task transfers only the minimum required capabilities
7. bootstrap task starts long-lived `init`
8. bootstrap task exits or transitions to a reduced role

---

## 12. Architectural Intent

This protocol deliberately enforces one principle:

**The first userspace task gets only what is needed to construct the next stage, not the whole machine.**

That is the line that preserves the microkernel architecture under real implementation pressure.

---

## 13. Future-Compatible Extensions

The following may be added later without changing the core principle:

- dedicated pager task split from bootstrap task
- dedicated resource manager
- explicit device-discovery service
- SMP startup coordination handoff
- richer module metadata
- signed boot modules
- stricter capability-right granularity
- measured boot / attestation metadata

None of these are part of the initial bootstrap contract.

---

## 14. Final Decision

For initial LumenOS bring-up, the bootstrap protocol is defined as follows:

- first userspace task: **`bootstrap-task`**
- first task image: **`bootstrap.elf`**
- initial objects: **Task, Thread, AddressSpace, BootInfo mapping, controlled boot memory VMOs, optional module VMOs**
- initial IPC object: **none**
- entry register: **`RDI = BootInfo*`**
- authority model: **explicit capability seeding only**
- excluded rights: **interrupts, devices, arbitrary physical memory, hidden privileged backchannel**

This is the normative v1 bootstrap contract.
