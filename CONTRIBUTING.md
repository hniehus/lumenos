# Contributing to LumenOS

This document sets contribution expectations for LumenOS. The project is an early-stage open-source operating system effort, and the repository should be treated as starter infrastructure for disciplined bring-up work rather than as a broad feature playground.

The current first target is narrow by design: `x86_64` on `QEMU` using `Limine`, with attention on early boot, handoff correctness, and kernel bring-up. Contributions should support that direction or improve the documentation and repository structure around it.

## Scope of contributions

Useful contributions at this stage include focused work on boot flow, kernel bring-up, architecture documentation, specifications, build scaffolding, and repository hygiene. Changes that introduce large new subsystems, policy-heavy abstractions, or speculative platform breadth before the first bring-up path is established should be avoided.

## Expectations for contributors

- Keep changes small, reviewable, and technically explicit.
- Prefer one clear purpose per commit and one focused concern per pull request.
- Explain what changed, why it changed, and how it was validated.
- Respect the current repository structure and the present bring-up target instead of designing for a much later system.

## Architecture discipline

LumenOS values clarity, minimalism, and deliberate architectural decisions. Avoid uncontrolled scope creep, convenience abstractions without a clear need, or temporary shortcuts that would become part of the long-term design.

If a change introduces a new abstraction, broadens the kernel boundary, alters the boot path, or changes an architectural assumption, document the rationale clearly in the pull request and update the relevant documentation in [`docs/`](docs) when appropriate.

## Reviewability and testing

Pull requests should stay compact enough to review carefully. When possible, include concrete validation notes appropriate to the current stage of the project, such as build status, `QEMU` boot behavior, and relevant serial output observations.

## Before Branching

The branching rules below define how work is expected to move through the repository. Read them before opening longer-lived work so the contribution shape and branch strategy stay aligned.

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
git push -u origin spike/uefi-loader
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
