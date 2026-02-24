# Build System Quick Start

**TL;DR: Use Docker for reproducible builds.**

## One-Minute Setup

### Option 1: Docker (Recommended)

```bash
# Build the container once
docker build -f Dockerfile -t lumenos-build .

# Then run builds
docker run --rm -v $(pwd):/workspace lumenos-build:latest make build
```

### Option 2: VS Code Dev Container

1. Install **Dev Containers** extension: `ms-vscode-remote.remote-containers`
2. Reopen project in container (Command Palette → "Reopen in Container")
3. Done! Everything is set up automatically

### Option 3: Manual Install (Not Recommended)

```bash
bash scripts/install-toolchain.sh --ubuntu
source .toolchain.env
make build
```

---

## What's Pinned?

See [**`toolchain.json`**](toolchain.json) for exact versions:

- **LLVM/Clang**: 18.1.8
- **LLD**: 18.1.8
- **NASM**: 2.15.05
- **Rust**: 1.75.0
- **QEMU**: 8.2.0+

All pinned in the **Dockerfile** and available in **Dev Container**.

---

## Documentation

Full details: **[`docs/dev/TOOLCHAIN.md`](docs/dev/TOOLCHAIN.md)**

Key topics:
- ✓ Why LLVM/Clang + LLD
- ✓ How versions are pinned
- ✓ Reproducible builds explained
- ✓ CI/CD pipeline
- ✓ Troubleshooting

---

## CI Pipeline

Every commit triggers:
1. ✓ Verify `toolchain.json` is valid
2. ✓ Build Docker image with pinned toolchain
3. ✓ Compile kernel inside container
4. ✓ Run tests in QEMU
5. ✓ Report build environment

**CI always uses the same versions** → Deterministic builds.

---

## Common Commands

| Task | Command |
|------|---------|
| Build in Docker | `docker run --rm -v $(pwd):/workspace lumenos-build make build` |
| Shell in Docker | `docker run -it -v $(pwd):/workspace lumenos-build bash` |
| Verify toolchain | `docker run --rm lumenos-build clang-18 --version && ld.lld-18 --version` |
| Install toolchain (host) | `bash scripts/install-toolchain.sh --ubuntu` |
| Check toolchain config | `cat toolchain.json` |

---

## Environment Variables

Used automatically in containers:

```bash
CC=clang-18
CXX=clang++-18
LD=ld.lld-18
AR=llvm-ar-18
OBJCOPY=llvm-objcopy-18
OBJDUMP=llvm-objdump-18
```

---

## Issues?

1. **"Clang: command not found"**: Use Docker instead of manual install
2. **"Different binary outputs"**: Rebuild inside Docker for consistency
3. **Multiple LLVM versions installed**: Docker isolates them automatically

**See [TOOLCHAIN.md](docs/dev/TOOLCHAIN.md#7-troubleshooting-toolchain-issues) for more help.**

---

For full details, see **[TOOLCHAIN.md](docs/dev/TOOLCHAIN.md)** and **[CONTRIBUTING.md](CONTRIBUTING.md)**.
