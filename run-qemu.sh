#!/usr/bin/env bash
set -euo pipefail

mkdir -p build
rm -f build/boot.log

status=0
timeout --foreground 15s qemu-system-x86_64 \
  -boot d \
  -cdrom build/lumenos.iso \
  -serial file:build/boot.log \
  -monitor none \
  -display none \
  -no-reboot || status=$?

if [ "$status" -ne 0 ] && [ "$status" -ne 124 ]; then
  exit "$status"
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
