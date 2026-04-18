# Pull Request

## Summary

This change extends the bring-up path from:

- bootloader -> kernel -> root task detected and validated

to:

- bootloader -> kernel -> root task ELF loadable segments mapped into a prepared image

The kernel now walks the root task ELF program headers, processes only
`PT_LOAD` segments, copies file-backed bytes into a dedicated prepared image,
zero-fills the remaining memory for each segment, and logs the resulting user
image layout without attempting execution.

## Changes

- preserve the existing kernel boot marker and root task module detection
- preserve minimal ELF validation for the root task module
- add a narrow root task image buffer preparation path in the kernel
- map only `PT_LOAD` segments
- copy segment file bytes and zero-fill the remainder up to `p_memsz`
- log segment count, each mapped segment range, the preserved entry point, and
  final image readiness
- harden `run-qemu.sh` to wait briefly for `build/boot.log` before reading it

## Expected Boot Markers

```text
kernel: early boot ok | version 0.1.0:000xx
kernel: modules request ok
kernel: module count = 1
kernel: root task module found
kernel: root task elf valid
kernel: root task map begin
kernel: root task load segments = N
kernel: map segment 0 vaddr=0x...
kernel: root task segments mapped
kernel: root task entry = 0x...
kernel: root task image ready
```

## Verification

```bash
make run
grep "kernel:" build/boot.log
```

## Notes

- no ring 3 transition is attempted yet
- no user thread is created yet
- no generalized VM subsystem or loader framework is introduced in this step
