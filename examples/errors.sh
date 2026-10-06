#!/usr/bin/env bash
# Check script loading and invalid CLI options.
set -u
./emulator --vfs examples/minimal.xml --script examples/missing.txt </dev/null || true
./emulator --script examples/config.txt --vfs examples/minimal.xml --bad 1 </dev/null || true
./emulator --vfs </dev/null || true
./emulator --vfs examples/missing.xml </dev/null || true
./emulator --vfs examples/invalid.xml </dev/null || true
