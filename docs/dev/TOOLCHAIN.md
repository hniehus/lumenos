# LumenOS Compiler Toolchain

This document defines the compiler toolchain, version pinning strategy, and reproducible build environment for LumenOS.

**Last Updated**: February 24, 2026  
**Version**: 1.0

## 1. Toolchain Choice

### Primary: LLVM/Clang + LLD

**Why LLVM/Clang?**
- **Consistency**: Single, modern project; no fragmentation between compiler and linker
- **Rust Interoperability**: Native LLVM backend; excellent for future Rust kernel components
- **Performance**: Comparable or better than GCC on x86-64
- **Active Development**: Frequent releases, cutting-edge optimization
- **Portability**: Better multi-architecture support (we may add ARM later)
- **C Standards**: Excellent C23 and C++ support (forward-compatible)

**Specific Versions** (see [`toolchain.json`](../../toolchain.json)):
- **LLVM**: 18.2.0
- **Clang**: 18.2.0
- **LLD**: 18.2.0
- **LLVM Tools**: llvm-ar, llvm-objcopy, llvm-objdump (version 18.2.0)

### Alternative: GCC + binutils

Not currently supported, but if needed in the future:
- **When**: If specific GCC-only optimizations prove necessary
- **Decision Point**: Milestone 2 or later
- **Migration Path**: Would require maintaining dual-compiler CI and compatibility testing

---

## 2. Version Pinning Strategy

### Purpose of Pinning
Allow builds to be **fully reproducible** across:
- Different developer machines
- CI/CD environments
- Build containers
- Over time (e.g., in 2 years)

### Where Versions Are Defined

**Machine-Readable Source of Truth**: [`toolchain.json`](../../toolchain.json)
- Contains all version numbers
- Includes release dates and rationale for each choice
- Used by scripts and containers

**The Dockerfile** (Primary Build Environment): [`Dockerfile`](../../Dockerfile)
- Pins Ubuntu base image (22.04 LTS, stable for 5+ years)
- Pins exact APT versions for LLVM and tools
- **All builds within Docker use identical versions**
- Docker image: `lumenos-build:latest` (tagged with git commit hash for production)

**Dev Container Configuration**: [`.devcontainer/devcontainer.json`](.devcontainer/devcontainer.json)
- Allows VS Code developers to use the same container
- Environment variables mirror Dockerfile

**Installation Script**: [`scripts/install-toolchain.sh`](../../scripts/install-toolchain.sh)
- For manual setup on host machines
- Parses `toolchain.json` to pin versions
- Supports gradual adoption (pinning can be optional)

### Version Format

```json
"component": {
  "version": "MAJOR.MINOR.PATCH",
  "version_full": "MAJOR.MINOR.PATCH-DISTRO",
  "reason": "Why this version"
}
```

### Updating Versions

When updating a toolchain component:

1. **Update `toolchain.json`** with new version and rationale
2. **Update `Dockerfile`** with new pinned version from APT (if applicable)
3. **Test locally** in Docker: `docker build -f Dockerfile -t lumenos-build:test .`
4. **Run full build** in container
5. **Commit** with message: `build: update LLVM to 19.0.0 (security fixes)`
6. **Version bump**: Increment `toolchain.json` top-level `version` field
7. **CI runs** new pinned versions automatically

---

## 3. Reproducible Build Environment

### Docker (Recommended)

The Dockerfile exactly reproduces the build environment. All developers and CI use this.

**Build the container:**
```bash
docker build -f Dockerfile -t lumenos-build:latest .
```

**Run a build inside the container:**
```bash
docker run --rm \
  -v $(pwd):/workspace \
  lumenos-build:latest \
  make build
```

**For interactive development:**
```bash
docker run --rm -it \
  -v $(pwd):/workspace \
  lumenos-build:latest \
  /bin/bash
```

**Production tagging** (CI will do this):
```bash
docker build -f Dockerfile -t lumenos-build:$(git rev-parse --short HEAD) .
```

### VS Code Dev Container

For developers using VS Code:

1. Install the **Dev Containers** extension (`ms-vscode-remote.remote-containers`)
2. Open the project
3. Click **Reopen in Container** (or run Command Palette → "Dev Containers: Reopen in Container")
4. VS Code automatically uses the Dockerfile and applies all environment variables

**Benefits:**
- Transparent: Just open the project as normal
- Isolated: No host machine pollution
- Integrated: Extensions, debugging, terminal all work inside container

### Manual Installation (Host Machine)

For developers who prefer native toolkits (not recommended for reproducibility but documented):

```bash
# Install from toolchain.json versions
bash scripts/install-toolchain.sh --ubuntu
```

**Important**: Manual installation carries risk of version drift. Always use Docker if possible.

---

## 4. Toolchain Environment Variables

All builds must set these environment variables (set automatically in containers):

```bash
CC=clang-18
CXX=clang++-18
LD=ld.lld-18
AR=llvm-ar-18
OBJCOPY=llvm-objcopy-18
OBJDUMP=llvm-objdump-18
```

### In Makefiles

```makefile
# Example: kernel/Makefile
CFLAGS := -target x86_64-none-elf \
          -march=x86-64 \
          -fno-builtin \
          -ffreestanding \
          -Wall -Wextra -Werror

# Compiler invocation uses env variables
$(KERNEL): $(OBJS)
	$(LD) -L$(SYSROOT)/lib -o $@ $(OBJS)
```

See [`kernel/Makefile`](../../kernel/Makefile) for full example.

---

## 5. CI/CD Pipeline

### GitHub Actions (.github/workflows/ci.yml)

The CI pipeline:

1. **Checks out source** (including `Dockerfile`, `toolchain.json`)
2. **Builds the container** with pinned versions
3. **Runs all builds** inside the container
4. **Artifacts**: Generated binaries have deterministic timestamps (via Dockerfile)
5. **Caches**: Docker layer caching speeds up repeated CI runs

**Current CI configuration:**

```yaml
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Build Docker image
        run: docker build -f Dockerfile -t lumenos-build:ci .
      - name: Build LumenOS
        run: docker run --rm -v $(pwd):/workspace lumenos-build:ci make build
      - name: Run tests
        run: docker run --rm -v $(pwd):/workspace lumenos-build:ci make test
```

### Determinism Guarantees

The Dockerfile ensures:
- ✅ **Same OS**: Ubuntu 22.04 (LTS, stable)
- ✅ **Same compiler**: LLVM 18.2.0 (exact version pinning)
- ✅ **Same linker**: LLD 18.2.0
- ✅ **Same assembler**: NASM 2.15.05
- ✅ **Same runtime**: Rust 1.75.0 (if used)
- ✅ **Same tools**: Make, pkg-config, etc.

**Test reproducibility:**

```bash
# Developer machine
make clean && make build
SHA256_A=$(sha256sum build/kernel.elf)

# Docker
docker run --rm -v $(pwd):/workspace lumenos-build:latest bash -c "make clean && make build"
SHA256_B=$(sha256sum build/kernel.elf)

# Should match (or be documented why they differ)
test "$SHA256_A" = "$SHA256_B" && echo "✓ Deterministic" || echo "✗ Diff detected"
```

---

## 6. Supported Operating Systems

### Primary Development Environments

- **Ubuntu 22.04 LTS** ← Recommended
- **Ubuntu 24.04 LTS**
- **Debian 12 Bookworm**

All three have LLVM packages available in official repositories.

### Other Platforms

| Platform | Status | Notes |
|----------|--------|-------|
| macOS | ⚠️ Partial | Docker Desktop works; native LLVM 18 available via Homebrew |
| Windows | ✅ Supported | WSL 2 + Docker or MSYS2 |
| Fedora/RHEL | ⚠️ Experimental | Use COPR or build LLVM from source |

**macOS Example:**

```bash
# Using Homebrew (if versions align)
brew install llvm@18 nasm make qemu

# Or use Docker Desktop (recommended for reproducibility)
docker build -f Dockerfile -t lumenos-build .
```

---

## 7. Troubleshooting Toolchain Issues

### "Clang: command not found"

The toolchain is not installed. Options:

1. **Use Docker (recommended):**
   ```bash
   docker build -f Dockerfile -t lumenos-build .
   docker run -it -v $(pwd):/workspace lumenos-build bash
   ```

2. **Install manually:**
   ```bash
   bash scripts/install-toolchain.sh --ubuntu
   ```

3. **Check PATH:**
   ```bash
   which clang-18
   echo $PATH
   ```

### "Different binary outputs" between runs

Could indicate:
- **Timestamp drift**: Rebuild inside Docker (has fixed timestamps)
- **Compiler version mismatch**: Verify `clang-18 --version` matches `18.2.0`
- **Optimization flags**: Check CFLAGS in Makefile consistency

**Solution:**
```bash
docker build -f Dockerfile -t lumenos-build .
docker run --rm -v $(pwd):/workspace lumenos-build make clean build
```

### LLVM version conflicts

If multiple LLVM versions are installed:

```bash
# These symbolic links are created in Dockerfile
# If installing manually, create them:
sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100
sudo update-alternatives --install /usr/bin/ld.lld ld.lld /usr/bin/ld.lld-18 100
```

---

## 8. Future Roadmap

### Planned Improvements

- **QEMU versioning**: Add to toolchain.json (currently uses host QEMU)
- **GDB pinning**: Add to Dockerfile for consistent debugging experience
- **Build cache**: Explore buildkit for faster incremental builds
- **Air-gapped builds**: Pre-baked Docker image with no network access
- **Multi-arch support**: Extend Dockerfile for ARM64 (future milestone)

### Quarterly Review

Toolchain versions are reviewed quarterly (or when CVEs are announced):

- **February 2026**: Reviewed, LLVM 18.2.0 approved
- **May 2026**: Scheduled review
- **August 2026**: Scheduled review
- **November 2026**: Scheduled review

---

## 9. References

- **LLVM**: https://releases.llvm.org/
- **NASM**: https://www.nasm.us/
- **Docker Documentation**: https://docs.docker.com/
- **Dev Containers**: https://containers.dev/
- Related ADRs: (TBD — Design decisions on architecture)

---

## 10. Quick Reference

| Task | Command |
|------|---------|
| **Build in Docker** | `docker run --rm -v $(pwd):/workspace lumenos-build:latest make build` |
| **Interactive Docker shell** | `docker run -it -v $(pwd):/workspace lumenos-build:latest bash` |
| **Use VS Code Dev Container** | Reopen in Container (Cmd Palette) |
| **Install toolchain (host machine)** | `bash scripts/install-toolchain.sh --ubuntu` |
| **Check versions** | `clang-18 --version && ld.lld-18 --version && nasm -version` |
| **Verify reproducibility** | `docker run --rm -v $(pwd):/workspace lumenos-build:latest make clean build && sha256sum build/kernel.elf` |
| **Update toolchain** | Edit `toolchain.json`, then update `Dockerfile` and test |

---

**Questions?** See [CONTRIBUTING.md](../../CONTRIBUTING.md) or open an issue on GitHub.
