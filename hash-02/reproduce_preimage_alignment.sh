#!/usr/bin/env bash
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
source_dir="$here/AXIS/Implementations/Reference_Implementation/AXIS-1024"
scratch_dir=$(mktemp -d)
trap 'rm -rf -- "$scratch_dir"' EXIT

cc -O2 -std=gnu11 -shared -fPIC -I"$source_dir" \
  "$here/preimage_alignment/harness.c" \
  -o "$scratch_dir/libaxis_harness.so"
export AXIS_HARNESS_LIB="$scratch_dir/libaxis_harness.so"
python3 "$here/preimage_alignment/check_model.py"
python3 "$here/preimage_alignment/e3_output_alignment.py" 1024
