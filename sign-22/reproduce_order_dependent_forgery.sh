#!/bin/sh
set -eu

commit=83360df0c060f54c327754a7baa3d7be10acfeeb
archive_sha256=4d617e489b4d807b38440fecbee0a00afbe0ab39d0944710c3e0743a18841fc9
scratch=$(mktemp -d "${TMPDIR:-/tmp}/ngcc-rhyme.XXXXXX")
trap 'rm -rf "$scratch"' EXIT HUP INT TERM

if ! curl -L --fail --silent --show-error \
    "https://codeload.github.com/martinfeussner/NGCC-Signature-Audit/tar.gz/$commit" \
    -o "$scratch/source.tar.gz"; then
    echo 'SKIP sign-22-4: pinned external artifact is unavailable' >&2
    exit 77
fi
printf '%s  %s\n' "$archive_sha256" "$scratch/source.tar.gz" | sha256sum -c -
tar -xzf "$scratch/source.tar.gz" -C "$scratch"
reproducer="$scratch/NGCC-Signature-Audit-$commit/Rhyme/reproducer"

if [ "${FULL:-0}" = 1 ]; then
    python=${PYTHON:-python3}
    PYTHON="$python" "$reproducer/run_full.sh" independent "$scratch/full"
else
    PYTHON="${PYTHON:-python3}" "$reproducer/verify_evidence.sh"
fi
