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
This repo expects a Linux host (Ubuntu 22.04+ recommended) with Docker or manually installed LLVM/Clang, NASM, and optional Rust.

**For reproducible builds, use Docker** (see [BUILD.md](BUILD.md)):
```bash
docker build -f Dockerfile -t lumenos-build .
docker run --rm -v $(pwd):/workspace lumenos-build make build
```

Alternatively, install toolchain manually:
```bash
bash scripts/install-toolchain.sh --ubuntu
```

### Build (placeholder)
For now, this is a scaffold. The build system will live in `tools/build/` and top-level `Makefile`.

Typical goals will look like:
- `make build`
- `make run`
- `make debug`

**📖 See [BUILD.md](BUILD.md) for quick setup and [docs/dev/TOOLCHAIN.md](docs/dev/TOOLCHAIN.md) for full details.**

## Contributing
See `CONTRIBUTING.md` (or `Contribution.md`) for guidelines.

## License
TBD (choose early to avoid legal ambiguity).
