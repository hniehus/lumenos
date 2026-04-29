# Pull Request

## Summary

This change introduces a minimal VMO object so memory is managed as an explicit
kernel object with identity, size, rights, and lifecycle tracking.

The kernel now models memory as:

- distinct VMOs with unique IDs and rights masks
- explicit mapping records per address space
- controlled map/unmap operations with overlap checks
- lifecycle logging for create, map, unmap, and drop events

The existing root-task bring-up path now uses VMOs for the prepared image and
stack, and a boot-time self-test exercises duplicate-mapping rejection and
cross-VMO overlap rejection plus unmapping cleanup before entering user mode.

## Changes

- add a tiny VMO object type with ID, size, rights, refcount, and mapping state
- add explicit address-space mapping records for VMO regions
- map the prepared root-task image through a root-task image VMO
- map the initial user stack through a stack VMO with a guard page below it
- add a boot self-test that:
  - maps a new VMO
  - rejects an overlapping mapping from a different VMO into the same address
    space
  - unmaps the VMO
  - releases the final reference and logs drop
- keep the existing user fault proof for kernel-address access
- document the VMO object and lifecycle rules in the bootstrap protocol spec

## Expected Boot Markers

```text
vmo: create id=1 size=0x...
vmo: create id=2 size=0x...
vmo: map id=2 as=1 vaddr=0x...
vmo: create id=3 size=0x...
vmo: cross-vmo overlap rejected
vmo: unmap id=3 as=1 vaddr=0x...
vmo: drop id=3
kernel: entering user mode
user: hello lumen
kernel: syscall handled
kernel: user fault page
kernel: user fault addr = 0xffffffff80000000
```

Guard-page smoke mode should instead fault at the stack guard address:

```text
kernel: user fault addr = 0x00007ffffffdf010
kernel: user fault code = 0x0000000000000006
```

## Verification

```bash
make smoke
make smoke-guard
```

## Notes

- duplicate overlapping mapping into the same address space is explicitly
  rejected
- file-backed VMOs, lazy population, shared-memory policy, and deduplication are
  intentionally out of scope
