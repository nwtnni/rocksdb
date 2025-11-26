#!/usr/bin/env bash

set -o pipefail
set -o nounset
set -o errexit
set -o xtrace

for num_threads in 1 2 4 8 16 32; do
    for memtablerep in skip_list arctic; do
        rm -rf bench/db/*
        rm -rf bench/wal/*

        NUM_KEYS=100000000 \
            NUM_THREADS=$num_threads \
            MEMTABLEREP=$memtablerep \
            CACHE_SIZE=$((2**30 * 6)) \
            DB_DIR=./bench/db \
            WAL_DIR=./bench/wal \
            OUTPUT_DIR=./bench/out \
            ./tools/benchmark.sh bulkload
    done
done
