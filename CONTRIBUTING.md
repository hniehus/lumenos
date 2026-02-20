# Contributing to LumenOS

## Ground rules
- Keep the system **clean**: prefer small, composable primitives over feature piles.
- No “quick hacks” that become permanent.
- If you introduce a new abstraction, you must explain why it’s needed.

## Workflow
1. Fork + create a feature branch
2. Keep commits small and descriptive
3. Open a PR with:
   - What changed
   - Why it changed
   - How to test it (QEMU steps, expected serial output, etc.)

## Coding standards
- Kernel C:
  - Avoid undefined behavior
  - No dynamic allocation in early boot paths unless explicitly designed
  - Prefer explicit error handling
- Rust:
  - `rustfmt` clean
  - No `unsafe` unless justified in comments and kept minimal
- Formatting:
  - C: `clang-format`
  - Shell: `shellcheck`

## Testing expectations
At minimum, changes should:
- Build successfully
- Boot in QEMU
- Not regress the serial log expectations for M1

## Reporting issues
When filing an issue, include:
- Host OS + versions (Ubuntu version, QEMU version)
- Repro steps
- Serial log output

## Security
If you find a security issue, do not post exploit details publicly. Create a private report (process TBD).

## Branching strategy

### Goals

*   **One stable line:** main always builds, tests pass, bootable (as far as CI can verify).
    
*   **Fast collaboration:** short-lived feature branches, clean PRs, predictable naming.
    
*   **Release discipline (later):** optional release/\* branches when you start tagging milestones.
    

### Branches

#### main (protected)

**Purpose:** always green, integration-ready.**Rules:**

*   No direct pushes.
    
*   PR required + CI required + at least 1 review.
    
*   Squash-merge by default (keeps history readable).
    

#### feature/\* (short-lived)

**Purpose:** new work (code, docs, tooling).**Naming:**

*   feature/\-
    
*   Examples:
    
    *   feature/kernel-scheduler
        
    *   feature/boot-limine
        
    *   feature/toolchain-clang
        

**Rules:**

*   Branch off main.
    
*   Rebase onto main frequently.
    
*   Merge via PR (squash).
    

#### fix/\* (short-lived)

**Purpose:** bug fixes targeting main.**Naming:** fix/\-Example: fix/mm-pagefault-null

#### spike/\* (time-boxed experiments)

**Purpose:** prototypes / research that may be thrown away.**Naming:** spike/Example: spike/virtio-net

**Rules:**

*   Not required to be “perfect”.
    
*   Must have a clear README or notes in the PR describing findings.
    
*   Prefer to merge as docs + minimal code, or close without merging.
    

#### Optional later: release/\*

Use **only** when you start cutting milestone releases.**Purpose:** stabilization + backports while main keeps moving.Example: release/m1

## Typical workflows

### Start a new feature

```bash
git checkout main
git pull --rebase origin main  
git checkout -b feature/kernel-scheduler  
# work...  
git status
git add -A
git commit -m "kernel: initial round-robin scheduler"
git push -u origin feature/kernel-scheduler   
```
Open a PR into main.

### Keep your branch up to date (preferred: rebase)

```bash
git fetch origin
git rebase origin/main
# resolve conflicts if needed
git push --force-with-lease
```
**Use --force-with-lease** (safe-ish), never plain --force.

### Quick fix branch

```bash
git checkout main
git pull --rebase origin main
git checkout -b fix/boot-triplefault
# work...
git commit -am "boot: fix GDT setup causing triple fault"
git push -u origin fix/boot-triplefault
```
PR into main.

### Spike / experiment

```bash
git checkout main
git pull --rebase origin main
git checkout -b spike/uefi-loader
# experiment...
git add -A  git commit -m "spike: uefi loader prototype + notes"
git push -u origin spike/uefi-loader   `
```
PR should clearly state whether it’s meant to merge or just document results.

## Commit message rules (keep it strict)

Format:

*   area: summaryExamples:
*   kernel: add preemptive timer tick
*   mm: implement buddy allocator
*   build: add clang+lld toolchain support
*   docs: document boot flow

## PR rules (non-negotiable)

*   CI must pass.
*   Keep PRs small and focused.
*   Include testing notes (even if it’s “Boots in QEMU: yes/no”).
*   Squash merge (default) to keep main clean.

## Maintainer-only: hotfix policy

If main is broken, fix it via a fix/\* branch + PR. No cowboy pushes.
