#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail
set -o xtrace

# https://stackoverflow.com/a/246128
ROOT=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# build arctic memtable
cd "$ROOT/memtable/arctic"
cargo build --release

cd "$ROOT"
mkdir -p build

cd "$ROOT/build"
cmake -DCMAKE_BUILD_TYPE=Release -DWITH_ZSTD=1 ..
make -j db_bench

cd "$ROOT"
[ -f db_bench ] || cp build/db_bench .
mkdir -p bench/db
mkdir -p bench/out
