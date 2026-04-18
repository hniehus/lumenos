# Kernel Versioning

LumenOS starts kernel versioning at:

- `MAJOR=0`
- `MINOR=1`
- `PATCH=0`

The full kernel version format is:

`MAJOR.MINOR.PATCH:BUILD`

Example:

`0.1.0:00001`

## How build numbering works

- The release components live in [`version/version.mk`](/home/harald/lumenos/version/version.mk).
- The build counter lives in [`version/build.counter`](/home/harald/lumenos/version/build.counter).
- Every `make kernel`, `make image`, or `make run` invocation that rebuilds the kernel increments the counter once.
- The current build writes the exact version string into `build/kernel_version.txt` and generates [`build/generated/version.h`](/home/harald/lumenos/build/generated/version.h) for the kernel source.

## Resetting or bumping versions

- Reset the build counter by editing `version/build.counter` back to `0`.
- Bump `MAJOR`, `MINOR`, or `PATCH` by editing `version/version.mk`.
- After changing `MAJOR`, `MINOR`, or `PATCH`, reset the build counter if you want the next build to start at `00001`.

## Expected boot marker

The earliest success marker is emitted over serial and captured in `build/boot.log`:

`kernel: early boot ok | version 0.1.0:00001`

For temporary terminal debugging, switch [`run-qemu.sh`](/home/harald/lumenos/run-qemu.sh) from `-serial file:build/boot.log` to `-serial stdio`.
