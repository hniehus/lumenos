# LumenOS

LumenOS is a new open-source operating system project. This repository currently serves as the starter foundation for early boot and kernel bring-up work, with an emphasis on architectural clarity and disciplined scope control.

## Status

LumenOS is in an early-stage system-project phase. This repository is intentionally minimal and should be read as a starting point for bring-up, architecture definition, and repository discipline rather than as a feature-complete operating system.

The current state is focused on establishing project structure, documenting architectural intent, and defining the constraints for the first executable path.

## Vision

LumenOS aims to grow into a clean, understandable operating system with explicit architectural boundaries, a controlled boot path, and a foundation that can evolve without accumulating accidental complexity.

## Current bring-up target

The first bring-up target is deliberately narrow:

- `x86_64` as the initial architecture
- `QEMU` as the execution and debugging environment
- `Limine` as the bootloader and boot protocol base
- early boot and kernel bring-up as the immediate engineering focus

This combination is a practical first target because it keeps hardware assumptions controlled, shortens the feedback loop during bring-up, and allows the boot interface to be specified clearly before broader platform work begins.

## Repository layout

The repository is still at an early stage, so some expected top-level areas are planned rather than present today.

- `bootsector/` (planned): experimental or educational low-level boot work; not the primary bring-up path
- `bootloader/` (planned): the intended real boot path for the current bring-up target
- [`docs/`](docs): architecture material and evolving technical specifications
- [`CONTRIBUTING.md`](CONTRIBUTING.md): contribution workflow and project rules
- `LICENSE` (not present yet): project license file once licensing is finalized

## Documentation

Current architecture documentation lives under [`docs/architecture/arc42/`](docs/architecture/arc42/), starting with [`docs/architecture/arc42/README.md`](docs/architecture/arc42/README.md).

The repository does not currently contain `documentation/specs/boot-contract.md` or `docs/specs/boot-contract.md`. For now, the boot contract direction is described in the arc42 architecture baseline, especially:

- [`docs/architecture/arc42/06-runtime-view.md`](docs/architecture/arc42/06-runtime-view.md)
- [`docs/architecture/arc42/09-architecture-decisions-and-open-questions.md`](docs/architecture/arc42/09-architecture-decisions-and-open-questions.md)

## Contributing

Contribution rules, workflow expectations, and branch discipline are defined in [`CONTRIBUTING.md`](CONTRIBUTING.md).

## License

A root `LICENSE` file has not been added yet. Licensing should be treated as undecided until that file is present in the repository.
