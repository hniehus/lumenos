#!/usr/bin/env bash
set -euo pipefail

mkdir -p build
rm -f build/boot.log

status=0
set +e
timeout --foreground 20s qemu-system-x86_64 \
  -boot d \
  -cdrom build/lumenos.iso \
  -serial stdio \
  -monitor none \
  -display none \
  -no-reboot 2>&1 | tee build/boot.log
status=${PIPESTATUS[0]:-1}
tee_status=${PIPESTATUS[1]:-0}
set -e

if [ "$status" -ne 0 ] && [ "$status" -ne 124 ]; then
  exit "$status"
fi

if [ "$tee_status" -ne 0 ]; then
  exit "$tee_status"
fi

for _ in 1 2 3 4 5; do
  if [ -f build/boot.log ]; then
    break
  fi
  sleep 1
done

if [ ! -f build/boot.log ]; then
  echo "run-qemu: boot log was not created" >&2
  exit 1
fi

cat build/boot.log
