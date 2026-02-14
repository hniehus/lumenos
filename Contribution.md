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
- Kernel C/C:
  - Avoid undefined behavior
  - No dynamic allocation in early boot paths unless explicitly designed
  - Prefer explicit error handling
- Rust:
  - `rustfmt` clean
  - No `unsafe` unless justified in comments and kept minimal
- Formatting:
  - C/C++: `clang-format`
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
