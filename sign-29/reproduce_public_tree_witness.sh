#!/bin/sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
source_dir="$repo_dir/sign-29/Implementations/Reference_Implementation/Tins256"
temp_dir=$(mktemp -d)
trap 'rm -rf -- "$temp_dir"' EXIT HUP INT TERM

cc -O2 -std=gnu11 -I "$source_dir" -Wl,--wrap=get_random_number \
  "$repo_dir/sign-29/reproduce_public_tree_witness.c" \
  "$source_dir/SIG_TINS256.c" "$source_dir/bavc_commit.c" \
  "$source_dir/ff_arith.c" "$source_dir/drng.c" "$source_dir/auxfunc.c" \
  -lm -o "$temp_dir/tins_public_tree_witness"
"$temp_dir/tins_public_tree_witness"
echo "CONFIRMED sign-29-7: Tins256 public-tree witness recovery and fresh-message forgery"
