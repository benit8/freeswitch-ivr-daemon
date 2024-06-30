#!/usr/bin/env bash
set -e

dir=$(cd "$(dirname "$0")" ; pwd)
root=$(dirname "$dir")

sources=`find src include -type f -name '*.c' -o -name '*.h'`

clang-format -i $sources
