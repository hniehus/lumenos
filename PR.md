# Pull Request

## Summary

This change introduces explicit capability rights bits and safe handle
delegation on top of the existing VMO model so object access is always
minimal, typed, and verifiable.

The kernel now models authority as:

- distinct capabilities with object type, rights mask, and validity state
- delegation that can preserve or reduce rights, but never expand them
- deterministic failures for invalid, wrong-type, or insufficient-rights access
- explicit VMO-backed mappings controlled through capability checks
- lifecycle logging for create, map, unmap, drop, and delegation events

The existing root-task bring-up path now uses capability-seeded VMOs for the
prepared image and stack, and a boot-time self-test exercises invalid access,
wrong-type access, insufficient rights, 1,000 non-escalating delegation cycles,
cross-VMO overlap rejection, and unmapping cleanup before entering user mode.

## Changes

- add a tiny VMO object type with ID, size, rights, refcount, and mapping state
- add a minimal capability object with object type, rights bits, validity, and
  delegation support
- add explicit address-space mapping records for VMO regions
- map the prepared root-task image through a root-task image VMO
- map the initial user stack through a stack VMO with a guard page below it
- add a boot self-test that:
  - maps a new VMO
  - rejects access without a matching capability
  - rejects access through the wrong object type
  - rejects access with insufficient rights
  - proves repeated delegation cannot escalate rights
  - rejects an overlapping mapping from a different VMO into the same address
    space
  - unmaps the VMO
  - releases the final reference and logs drop
- keep the existing user fault proof for kernel-address access
- document the VMO object, capability rights bits, and delegation rules in the
  bootstrap protocol spec and architecture docs

## Expected Boot Markers

```text
vmo: create id=1 size=0x...
vmo: create id=2 size=0x...
vmo: map id=2 as=1 vaddr=0x...
vmo: create id=3 size=0x...
cap: invalid access code=1
cap: wrong-type access code=2
cap: insufficient-rights access code=4
cap: attenuation cycles complete count=1000
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
- capability delegation is attenuation only and cannot increase rights
- file-backed VMOs, lazy population, shared-memory policy, and deduplication are
  intentionally out of scope
