#!/bin/bash
# Relink one built instance with the ICCS wrappers (wrap.c) into
#   performance/hashprof/lib/<candidate>/lib<label>.so
# reusing the harness's own link command for that instance; the candidate's
# lib/lib<label>.so is left untouched. Only the ICCS entry points the objects
# actually define are wrapped (some candidates do not link auxfunc.c at all).
#
#   relink.sh CANDIDATE LABEL [MAKEFILE [MAKE_ARGS...]]
set -euo pipefail
H=$(cd "$(dirname "$0")" && pwd); R=$(cd "$H/../.." && pwd)
cand=$1; label=$2; mkfile=${3:-Makefile}; shift $(( $# < 3 ? $# : 3 ))
line=$(make -n -B -C "$R/$cand" -f "$mkfile" "$@" "lib/lib$label.so" 2>/dev/null \
       | grep -E -- "-shared .* -o lib/lib$label\.so( |$)" | tail -1)
[ -n "$line" ] || { echo "relink: no link command for $cand $label" >&2; exit 1; }
mkdir -p "$H/lib/$cand" "$H/build"
# concurrent relinks (campaign.py --shard) replace shared files atomically
if [ ! "$H/build/wrap.o" -nt "$H/wrap.c" ]; then
    cc -O2 -fPIC -c "$H/wrap.c" -o "$H/build/wrap.o.$$" && mv -f "$H/build/wrap.o.$$" "$H/build/wrap.o"
fi
# the official export list plus the profiling interface
sed 's/^    CryptHash;/    CryptHash;\n    ngcc_hashprof_tick_source; ngcc_hashprof_ticks; ngcc_hashprof_enable;\n    ngcc_hashprof_reset; ngcc_hashprof_entries;/' \
    "$R/api/link_exports.map" > "$H/build/hashprof.map.$$" && mv -f "$H/build/hashprof.map.$$" "$H/build/hashprof.map"
objs=$(grep -o 'build/[^ ]*\.o' <<<"$line" || true)
defined=$(cd "$R/$cand" && nm --defined-only $objs 2>/dev/null | awk '{print $3}')
wraps=""
syms="pseudohash pseudoXOF sm3hash get_random_number"
# HASHPROF_SHA3=1: also the pq-crystals SHA-3 entry points (performance/supplement)
if [ "${HASHPROF_SHA3:-0}" = 1 ]; then
    for ns in pqcrystals_kyber_fips202_ref_ pqcrystals_dilithium_fips202_ref_; do
        for f in shake128_init shake128_absorb shake128_finalize shake128_squeeze shake128_absorb_once \
                 shake128_squeezeblocks shake256_init shake256_absorb shake256_finalize shake256_squeeze \
                 shake256_absorb_once shake256_squeezeblocks shake128 shake256 sha3_256 sha3_512; do
            syms="$syms $ns$f"
        done
    done
fi
for f in $syms; do
    grep -qx "$f" <<<"$defined" && wraps="$wraps -Wl,--wrap=$f"
done
# the export map, whatever the relative path to api/ (candidates use ../api)
line=$(sed -E "s#--version-script=[^ ]*api/link_exports\.map#--version-script=$H/build/hashprof.map#" <<<"$line")
line=${line//-o lib\/lib$label.so/-o $H\/lib\/$cand\/lib$label.so $H\/build\/wrap.o $wraps}
(cd "$R/$cand" && eval "$line")
echo "$H/lib/$cand/lib$label.so"
