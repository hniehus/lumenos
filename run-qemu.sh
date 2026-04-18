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

cat build/boot.log
