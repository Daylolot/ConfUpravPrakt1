#!/usr/bin/env bash
# Run from the project root after make.
set -u
for file in minimal files deep; do
    echo "=== $file ==="
    ./emulator --vfs "examples/$file.xml" --script examples/vfs.txt </dev/null || true
done
echo '=== no VFS ==='
./emulator --script examples/vfs.txt </dev/null || true
