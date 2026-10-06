#!/usr/bin/env bash
set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_dir="$script_dir/Implementation/Reference_Implementation/lwrdsa512"
if ! command -v cc >/dev/null 2>&1; then
  echo 'SKIP sign-15-13: C compiler unavailable' >&2
  exit 77
fi
if ! printf '#include <flint/flint.h>\n' | cc -E -x c - -o /dev/null >/dev/null 2>&1; then
  echo 'SKIP sign-15-13: FLINT headers unavailable' >&2
  exit 77
fi
scratch_dir=$(mktemp -d /tmp/ngcc-atlas512-XXXXXX)
trap 'case "$scratch_dir" in /tmp/ngcc-atlas512-*) rm -rf -- "$scratch_dir";; esac' EXIT

cc -O2 -std=c99 -I "$source_dir" -o "$scratch_dir/atlas" \
  "$script_dir/reproduce_512_nonce_overlap.c" \
  "$source_dir/SIG_lwrdsa512.c" \
  "$source_dir/polyvec.c" \
  "$source_dir/packing.c" \
  "$source_dir/poly.c" \
  "$source_dir/rounding.c" \
  "$source_dir/auxfunc.c" \
  "$source_dir/drng.c"
cc -O2 -std=c11 -o "$scratch_dir/recover" \
  "$script_dir/recover_512_nonce_overlap.c" -lflint -lgmp

"$scratch_dir/atlas" --collect > "$scratch_dir/public-transcript"
"$scratch_dir/recover" "$scratch_dir/public-transcript" | "$scratch_dir/atlas" --forge
printf 'CONFIRMED sign-15-13: ATLAS-512 nonce-overlap key recovery and fresh-message forgery\n'
