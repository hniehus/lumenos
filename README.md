# LumenOS

LumenOS is an open-source, from-scratch desktop operating system targeting **x86-64** first, designed to be **clean**, **lightweight**, and **high-performance**.

## Goals (high level)
- Modern, capability-oriented security model (no “everything is global”)
- Fast boot + responsive desktop latency (input-to-frame focus)
- SMP + preemptive multitasking from day one
- Clear separation of **mechanisms (kernel)** and **policies (userspace services)**
- Portability-friendly architecture (x86-64 first, easy to add new architectures later)

## Milestone 1 (M1) definition
**Boot → SMP → Scheduler → Userspace init → Minimal shell**

Out of scope for M1:
- Full driver stack (beyond what is needed for boot + serial)
- Graphics stack / compositor
- Networking

## Repository structure
- `kernel/` — kernel sources (arch, mm, sched, ipc, drivers, etc.)
- `user/` — userspace programs (`init`, `shell`, minimal libc/runtime)
- `tools/` — build/CI/debug helpers
- `docs/` — architecture decisions (ADR), design notes, developer docs
- `scripts/` — utility scripts
- `.github/` — CI workflows

## Quick start (Ubuntu)
### Requirements
This repo expects a Linux host (Ubuntu recommended) with QEMU, OVMF, LLVM/Clang, NASM, and Rust.

If you used the bootstrap script, most dependencies are already installed.

### Build (placeholder)
For now, this is a scaffold. The build system will live in `tools/build/` and top-level `Makefile`.

Typical goals will look like:
- `make build`
- `make run`
- `make debug`

## Contributing
See `CONTRIBUTING.md` (or `Contribution.md`) for guidelines.

## License
TBD (choose early to avoid legal ambiguity).
