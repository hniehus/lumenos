# ADR-001: Primary Compiler Toolchain Selection

**Status**: Accepted  
**Date**: February 24, 2026  
**Deciders**: LumenOS Core Team  
**Affected**: All developers, CI/CD pipeline, build system

---

## Context

LumenOS requires a compiler toolchain to build:
- **x86-64 kernel** in C with inline assembly
- **Userspace utilities** in C and Rust
- **Boot code** in assembly (NASM)

The choice of toolchain affects:
- **Development experience**: Compiler warnings, error messages, IDE support
- **Reproducibility**: Version stability, availability across platforms
- **Performance**: Optimization capabilities, code generation quality
- **Maintenance**: Community support, frequency of updates
- **Compatibility**: With existing build tools, debuggers, and runtime environments

We needed to evaluate and decide between:
1. **LLVM/Clang + LLD** (modern, unified)
2. **GCC + binutils** (traditional, mature)

---

## Decision

We choose **LLVM/Clang + LLD** as the primary compiler toolchain.

**Specific versions (pinned in `toolchain.json`)**:
- LLVM: 18.2.0
- Clang: 18.2.0
- LLD: 18.2.0
- NASM: 2.15.05

---

## Rationale

### 1. Unified Ecosystem
- **LLVM/Clang/LLD** are all part of one project with consistent APIs and philosophies
- No fragmentation between compiler and linker design
- Easier debugging of cross-tool issues

### 2. Rust Interoperability
- Clang uses LLVM backend → Native compatibility with Rust's LLVM-based compilation
- Critical for future kernel components or utilities in Rust
- No impedance mismatch between C and Rust compilation units

### 3. Modern C/C++ Support
- Excellent C23 support (forward-compatible for later standards)
- Good diagnostics and error messages
- Better undefined behavior detection than older GCC versions

### 4. Cross-Platform Consistency
- LLVM available on Linux, macOS, Window (via WSL)
- Better chance of reproducible builds across different host machines
- Container environments (Docker) have stable LLVM packages

### 5. x86-64 Backend Quality
- Comparable or better optimization than GCC on x86-64
- Good support for inline assembly and low-level intrinsics
- Mature support for kernel-specific attributes (noreturn, section, etc.)

### 6. Active Development
- LLVM 18 released March 2024, LTS versions available
- Quarterly major releases, predictable release cycle
- Strong community and corporate backing (Google, Apple, Amazon, Meta)

### 7. Tooling Ecosystem
- `llvm-ar`, `llvm-objcopy`, `llvm-objdump` all part of same release
- Can eliminate dependency on GNU binutils (optional)
- Better integration with debuggers (LLDB, GDB)

---

## Considered Alternatives

### Alternative 1: GCC + binutils

**Pros**:
- More mature, stable for decades
- Familiar to many kernel developers
- Slightly smaller toolchain size

**Cons**:
- Separate projects (GCC, binutils, glibc, …) with different release cycles
- Not ideal for Rust interop (Rust's primary backend is LLVM)
- Less active on modern language standards
- Would require maintaining two build paths if we later add Rust kernel code

**Decision**: Rejected for primary, kept as future alternative if needed.

### Alternative 2: Intel ICC (Intel Compiler)

**Pros**:
- Excellent x86-64 optimization

**Cons**:
- Closed source, proprietary
- Not available for free in enterprise settings
- Small community, harder to find help
- Incompatible with open-source expectations

**Decision**: Rejected.

---

## Consequences

### Positive
✓ **Deterministic builds** through pinned toolchain versions  
✓ **Rust-ready** for future components  
✓ **Modern C23 support** for advanced language features  
✓ **Container-native** (Dockerfile provides reproducible environment)  
✓ **CI/CD simplification** (single toolchain to maintain)

### Negative
⚠ **Learning curve** for developers unfamiliar with Clang diagnostics  
⚠ **Potential compatibility issues** if code relies on GCC extensions  
⚠ **LLVM updates can be frequent** (but we pin versions)

### Mitigation
- **Documentation**: [docs/dev/TOOLCHAIN.md](../docs/dev/TOOLCHAIN.md) provides setup and troubleshooting
- **Pinning**: [toolchain.json](../toolchain.json) ensures all builds use same versions
- **Docker**: Dockerfile provides isolated, reproducible environment
- **Testing**: CI runs with pinned toolchain every commit
- **Alternatives**: GCC support can be added later if needed (low cost)

---

## Implementation Status

✓ **Completed**:
- `toolchain.json` pinned versions
- Dockerfile with LLVM 18.2.0
- `.devcontainer/devcontainer.json` for VS Code
- `scripts/install-toolchain.sh` for manual setup
- Updated CI/CD pipeline (`.github/workflows/ci.yml`)
- Documentation: [docs/dev/TOOLCHAIN.md](../docs/dev/TOOLCHAIN.md)

⚠ **Deferred** (Future milestones):
- GCC support (if needed for specific optimizations or compatibility)
- Cross-compilation toolchains (ARM64, RISC-V)
- Offline toolchain distribution (air-gapped builds)

---

## How to Use

### For Developers
```bash
# Option 1: Use Docker (recommended)
docker build -f Dockerfile -t lumenos-build .
docker run --rm -v $(pwd):/workspace lumenos-build make build

# Option 2: VS Code Dev Container
# Reopen in Container (Command Palette)

# Option 3: Manual install
bash scripts/install-toolchain.sh --ubuntu
source .toolchain.env
make build
```

### For CI/CD
All GitHub Actions workflows use the Dockerfile, ensuring every build uses LLVM 18.2.0.

---

## References

- **LLVM Project**: https://llvm.org/
- **Clang**: https://clang.llvm.org/
- **LLD**: https://lld.llvm.org/
- **Rust LLVM Backend**: https://doc.rust-lang.org/rustc/platform-support.html
- **Docker**: https://docs.docker.com/
- **Related**: [BUILD.md](../BUILD.md), [CONTRIBUTING.md](../CONTRIBUTING.md)

---

## Questions / Follow-up

- **Q: What if I need GCC?**  
  A: You can create an alternative `Dockerfile.gcc`, but CI/CD will remain on Clang. Contact core team.

- **Q: Can I use a different LLVM version?**  
  A: Locally, yes. But for reproducible builds and CI, use the pinned version in `toolchain.json`.

- **Q: What about on macOS?**  
  A: Use Docker Desktop or Homebrew's LLVM 18.

- **Q: Future roadmap for this decision?**  
  A: Quarterly reviews of LLVM versions (security, performance). Alternative toolchains reconsidered at Milestone 2+.
