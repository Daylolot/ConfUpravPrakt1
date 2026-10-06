#!/usr/bin/env bash
# Run from the project root after make.
set -u
./emulator --vfs examples/minimal.xml --script examples/config.txt </dev/null || true
./emulator --script examples/config.txt --vfs examples/minimal.xml </dev/null || true
./emulator --vfs examples/minimal.xml </dev/null <<<'conf-dump'
