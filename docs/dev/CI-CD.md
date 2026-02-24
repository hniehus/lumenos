# CI/CD Pipeline Documentation

**Source of Truth for Reproducible Builds**

This document explains how the CI/CD pipeline uses the pinned toolchain to ensure reproducible builds.

---

## Overview

The GitHub Actions CI pipeline (`.github/workflows/ci.yml`) builds LumenOS using a **Docker image with pinned toolchain versions**.

**Key principle**: CI runs the exact same build environment on every commit, producing deterministic output.

---

## Pipeline Stages

### 1. **toolchain-verify** (Fast)
- Validates `toolchain.json` is valid JSON
- Checks Dockerfile structure is correct
- ✓ Catches configuration errors early

### 2. **build-image** (Medium)
- Builds Docker image from `Dockerfile`
- Caches LLVM 18.1.8 and dependencies
- Displays toolchain version report
- Uses GitHub Actions cache to speed up rebuilds

### 3. **build** (Main work)
- Runs `make build` inside the container
- Uses pinned LLVM 18.1.8, LLD, NASM, etc.
- Currently placeholder (TODO: actual kernel build)
- **Outputs**: Kernel ELF, binary, bootable image

### 4. **test** (Emulation)
- Runs QEMU with compiled kernel
- Tests boot, scheduler, userspace init
- Currently placeholder (TODO: test suite)
- Must pass before PR merge

### 5. **coverage** (Report)
- Displays toolchain versions in build summary
- Reports to GitHub commit status
- Helps track which versions built each commit

---

## Dockerfile as "Source of Truth"

The `Dockerfile` pins:

```dockerfile
# Ubuntu base (stable)
FROM ubuntu:22.04

# LLVM 18.1.8
apt-get install clang-18=1:18.1.8-1~ubuntu0.22.04.1

# LLD 18.1.8
apt-get install lld-18=1:18.1.8-1~ubuntu0.22.04.1

# Other tools (exact versions)
apt-get install nasm=2.15.05-1 make=4.3-4.1

# Environment variables set
ENV CC=clang-18 CXX=clang++-18 LD=ld.lld-18 ...
```

Every container built from this Dockerfile is **identical**.

---

## How to Manually Run CI Locally

### Option 1: Run full CI pipeline locally

```bash
# Build Docker image
docker build -f Dockerfile -t lumenos-build:local .

# Step 1: Toolchain verify
docker run --rm lumenos-build:local \
  python3 -c "import json; json.load(open('/workspace/toolchain.json'))"

# Step 2: Build
docker run --rm -v $(pwd):/workspace lumenos-build:local \
  bash -c "cd /workspace && make build"

# Step 3: Test (placeholder)
docker run --rm -v $(pwd):/workspace lumenos-build:local \
  bash -c "cd /workspace && make test"
```

### Option 2: Run single CI job

```bash
# Just build the kernel
docker run --rm -v $(pwd):/workspace lumenos-build:local make build
```

---

## Debugging CI Failures

### "Build failed in CI but works locally"

**Root cause**: Your local environment differs from Docker.

**Fix**: Test in Docker before pushing:
```bash
docker build -f Dockerfile -t test .
docker run --rm -v $(pwd):/workspace test make clean build
```

### "Docker image build failed"

**Check Dockerfile syntax**:
```bash
docker build -f Dockerfile --dry-run .
```

**Check specific layer**:
```bash
# Build up to layer N
docker build -f Dockerfile --target lumenos-build .
```

### "Toolchain versions differ between CI runs"

**This shouldn't happen** (version are pinned), but if it does:
1. Check Docker image tag changed
2. Verify `toolchain.json` is checked in
3. Check Dockerfile APT versions are exact

---

## Viewing CI Artifacts

After a CI run, view outputs:
1. Go to **GitHub repo → Actions → [Workflow run]**
2. Click **build-image** job → scroll to version report
3. Check logs for any errors
4. Download artifact (when implemented)

---

## Modifying CI Pipeline

### Adding a new build step

Edit `.github/workflows/ci.yml`:

```yaml
- name: My new step
  run: |
    docker run --rm -v $(pwd):/workspace lumenos-build:latest \
      bash -c "cd /workspace && make my-target"
```

### Updating toolchain version

1. Edit `toolchain.json` with new version
2. Update `Dockerfile` APT package version
   ```dockerfile
   apt-get install clang-18=1:18.2.0-1~ubuntu0.22.04.1
   ```
3. Test locally:
   ```bash
   docker build -f Dockerfile -t lumenos-build:test .
   docker run --rm -v $(pwd):/workspace lumenos-build:test clang-18 --version
   ```
4. Commit and push (CI will use new versions automatically)

### Caching and Performance

GitHub Actions caches Docker layers automatically:
- First run: Builds full image (~2-3 min)
- Subsequent runs: Uses cached layers (~30 sec)
- Cache invalidates if Dockerfile changes

To force a rebuild (skip cache):
```bash
# In .github/workflows/ci.yml, add to build-push-action:
cache: false
```

---

## Secrets and Credentials

Currently: None needed (all open-source).

If needed later:
- Store in GitHub repo Settings → Secrets
- Reference as `${{ secrets.SECRET_NAME }}`
- Never commit credentials

---

## Notifications

Currently: None configured.

GitHub automatically:
- ✓ Posts PR status (green / red)
- ✓ Blocks merge if CI fails
- ✓ Shows logs on failure

Future options:
- Slack/Discord integration
- Email notifications
- Custom webhooks

---

## Performance Targets

| Job | Target | Notes |
|-----|--------|-------|
| toolchain-verify | <1s | JSON validation only |
| build-image | 1-2 min | (fast if cached) |
| build | 2-5 min | (TODO: kernel build) |
| test | 5-10 min | (TODO: QEMU boot test) |
| **Total** | **~10-20 min** | First run; much faster (cached) |

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| CI hangs | Check QEMU timeouts; add explicit timeout to steps |
| Out of disk space | GitHub Actions has 14 GB; clean old runs |
| Build slow on first run | Normal; Docker layer caching speeds up later runs |
| Image not found | Ensure `docker build` completes successfully |
| Script permission denied | Add `chmod +x` to scripts before use |

---

## Related

- [BUILD.md](../BUILD.md) — Quick start for local builds
- [docs/dev/TOOLCHAIN.md](../docs/dev/TOOLCHAIN.md) — Toolchain details
- [CONTRIBUTING.md](../CONTRIBUTING.md) — Development workflow
- [toolchain.json](../toolchain.json) — Pinned versions
- [Dockerfile](../Dockerfile) — Build environment

---

## References

- **GitHub Actions**: https://docs.github.com/en/actions
- **Docker**: https://docs.docker.com/
- **Ubuntu Launchpad**: https://launchpad.net/ubuntu/ (for package versions)
