# ADR 0004: Boot Contract

- **Status:** Accepted
- **Date:** 2026-04-16
- **Deciders:** LumenOS architecture team

## Context

The first real execution boundary in LumenOS is not “kernel starts somehow.” It is the contract between bootloader and kernel.

Without a precise boot contract, the kernel will silently depend on accidental machine state, undocumented assumptions, or bootloader quirks. That makes bring-up fragile and later evolution expensive.

The project therefore needs an explicit, implementable boot contract for the initial platform.

## Decision

LumenOS defines a **formal boot contract** between the initial bootloader baseline and the kernel.

For the initial bring-up target, this contract is based on:

- **Architecture:** x86_64
- **Execution environment:** QEMU
- **Bootloader baseline:** Limine

The boot contract defines:

1. what the bootloader is responsible for,
2. what data must be handed to the kernel,
3. what machine state the kernel may assume at entry,
4. what the kernel must establish for itself after entry.

## Bootloader responsibilities

The bootloader **MUST**:

1. load the kernel image,
2. prepare the execution environment required by the agreed boot protocol,
3. provide the agreed handoff data to the kernel,
4. transfer control only through the defined kernel entry contract.

The bootloader is **NOT** responsible for higher-level kernel initialization, policy decisions, or user-space bootstrap beyond the defined handoff.

## Required handoff data

The boot contract **MUST** provide the following handoff data to the kernel for the initial baseline:

| Handoff element | Requirement |
|---|---|
| Physical memory map | Required |
| Kernel image ranges / occupied image regions | Required |
| RSDP / ACPI pointer | Required when available and needed early |
| SMP / CPU information | Required when provided by the boot protocol |
| Framebuffer information | Optional; not a bring-up priority |

The kernel **MUST NOT** assume additional handoff data unless it is explicitly included in the contract.

## Kernel entry assumptions

At kernel entry, the following assumptions are part of the contract for the initial baseline:

| Entry condition | Contract |
|---|---|
| CPU mode | x86_64 long mode is active |
| Paging | Active according to the boot protocol and documented handoff state |
| Initial stack | A valid entry stack exists and may be used for early kernel entry |
| Non-documented state | Must not be relied on by the kernel |

The kernel **MUST** treat any state not guaranteed by the contract as undefined.

## Kernel responsibilities after entry

After receiving control, the kernel **MUST**:

1. validate and consume the handoff state,
2. establish its own early runtime assumptions,
3. initialize only the essential memory and execution structures required for bootstrap,
4. avoid reliance on hidden firmware or bootloader state beyond the contract,
5. continue into the kernel bootstrap path that eventually creates the first user-space context.

## Explicit non-goals

The following are **not** part of the required first boot contract:

- rich graphics support
- complex framebuffer-driven startup
- broad device enumeration beyond what is needed for bootstrap
- higher-level service startup policy

Framebuffer support is allowed but remains optional and secondary for the first bring-up.

## Normative rules

1. The boot contract **MUST** be explicit, documented, and testable.
2. The kernel **MUST** assume only what the contract guarantees.
3. The bootloader **MUST NOT** become a shadow runtime for the kernel.
4. The contract **SHOULD** remain minimal and stable during early bring-up.
5. Any expansion of the handoff payload **MUST** be documented in a follow-up ADR or protocol revision.

## Rationale

This decision turns boot into an engineering contract instead of folklore. It creates a stable boundary for bring-up, reduces accidental coupling, and makes later refactoring safer.

It also provides a clean architectural seam between:

- boot image construction,
- bootloader behavior,
- kernel initialization,
- later user-space bootstrap.

## Consequences

### Positive

- Clear separation of responsibilities
- Less accidental dependence on bootloader quirks
- Better testability of kernel entry behavior
- Cleaner path to a later bootstrap protocol
- Easier reasoning about what the kernel may trust

### Negative

- More up-front specification work
- Some convenience shortcuts during bring-up are intentionally disallowed

## Rejected alternatives

### Let the kernel depend on whatever state the bootloader currently happens to provide

Rejected because it creates fragile and undocumented coupling.

### Make framebuffer support a first-class boot requirement immediately

Rejected because visual richness is not the first proof point; architectural correctness is.
