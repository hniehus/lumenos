# 9. Architecture Decisions and Open Questions

## 9.1 Decisions Already Anchored

- Start on **x86_64 under QEMU**
- Define the first fastpath early
- Define a minimal kernel object model immediately
- Write a Kernel Charter to freeze kernel boundaries before drift begins
- Define a **formal boot protocol and handoff contract**
- Define a **formal bootstrap protocol** from early kernel state to init
- Build and prove a **minimal boot path** before growing richer boot-time behavior

## 9.2 Decisions That Need Formal ADRs Next

- ratify kernel type explicitly
- define syscall ABI shape
- define IPC semantics precisely
- define capability representation and transfer model
- define initial scheduler model
- define what exactly DeviceHandle abstracts in the first cut
- define the concrete boot contract structure and payload schema
- define the concrete init bootstrap context and handoff representation

## 9.3 Open Questions

- What exact handoff payload fields are mandatory at kernel entry in the first version?
- Does the minimal boot path include an initrd-like payload in the first cut or only kernel-plus-init reachability?
- Should interrupts be deliverable through a capability-mediated user-space path in the first version or only later?
- How much of device management belongs in the first user space?
- Is the first scheduler strictly single-core oriented, with SMP deferred entirely?
- Which parts of memory management are essential before user-space initialization, and which can wait?

---
