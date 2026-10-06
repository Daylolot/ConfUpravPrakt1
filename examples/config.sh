#!/usr/bin/env bash
# Run from the project root after make.
set -u
./emulator --vfs examples/future-vfs.xml --script examples/config.txt </dev/null || true
./emulator --script examples/config.txt --vfs examples/future-vfs.xml </dev/null || true
./emulator --vfs examples/future-vfs.xml </dev/null <<<'conf-dump'
