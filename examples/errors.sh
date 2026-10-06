#!/usr/bin/env bash
# Check script loading and invalid CLI options; the VFS path is only stored on stage 2.
set -u
./emulator --vfs examples/future-vfs.xml --script examples/missing.txt </dev/null || true
./emulator --script examples/config.txt --vfs examples/future-vfs.xml --bad 1 </dev/null || true
./emulator --vfs </dev/null || true
