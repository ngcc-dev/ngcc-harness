#!/bin/sh
# Reproduce every demonstrated defect reported in this repository.
#
#   make -C api harness          # once
#   make -C <candidate>          # build the candidate libraries you want
#   tools/reproduce.sh           # run every reproducer
#   tools/reproduce.sh hash-09   # just one candidate
#
# Each line is either
#   <report-id> ATTACK <check> <instance> CONFIRMED ...      the defect is present
#   <report-id> ATTACK <check> <instance> NOT-CONFIRMED ...  it is not
# Control rows are marked [control] and are EXPECTED to print NOT-CONFIRMED:
# they run the same check against an unaffected candidate to show the test
# itself is sound.
#
# Exit status: 0 if every selected finding reproduced, every selected control
# stayed clean, and no required library was missing.

set -u
cd "$(dirname "$0")/.." || exit 2
A=tools/ngcc_attack
S=security/ngcc_security
[ -x "$A" ] || { echo "build first: make -C tools" >&2; exit 2; }

only="${1:-}"
fail=0
skipped=0

# run <candidate> <label> <expect CONFIRMED|NOT-CONFIRMED> <args...>
run() {
    cand=$1; label=$2; expect=$3; shift 3
    [ -n "$only" ] && [ "$only" != "$cand" ] && return 0
    # the library argument is the first remaining one after the check name
    lib=$2
    if [ ! -f "$lib" ]; then
        echo "SKIP   $cand $label (build it: make -C $cand)"
        skipped=$((skipped + 1))
        return 0
    fi
    out=$("$A" "$@" 2>&1)
    rc=$?
    case "$out" in
        *CONFIRMED*) got=CONFIRMED ;;
        *) got=ERROR ;;
    esac
    case "$out" in *NOT-CONFIRMED*) got=NOT-CONFIRMED ;; esac
    if [ "$got" = "$expect" ]; then
        printf '%s  %s\n' "$label" "$out"
    else
        printf 'UNEXPECTED (%s, wanted %s) %s\n' "$got" "$expect" "$out"
        fail=$((fail + 1))
    fi
    return 0
}

# run_target <candidate> <report-id(s)> <command...>
run_target() {
    cand=$1; label=$2; shift 2
    [ -n "$only" ] && [ "$only" != "$cand" ] && return 0
    "$@"
    rc=$?
    if [ "$rc" -eq 0 ]; then
        echo "$label  REPRODUCER PASS"
    elif [ "$rc" -eq 77 ]; then
        echo "SKIP   $cand $label (dependency or pinned artifact unavailable)"
        skipped=$((skipped + 1))
    else
        echo "UNEXPECTED $label reproducer failure"
        fail=$((fail + 1))
    fi
}

# find_python <probe>
# Print a Python executable that satisfies the supplied import/version probe.
# NGCC_ESTIMATOR_PYTHON is intended for isolated estimator environments;
# NGCC_SAGE_PYTHON remains a compatibility fallback for existing setups.
find_python() {
    probe=$1
    configured=${NGCC_ESTIMATOR_PYTHON:-${NGCC_SAGE_PYTHON:-}}
    if [ -n "$configured" ] && "$configured" -c "$probe" >/dev/null 2>&1; then
        printf '%s\n' "$configured"
    elif python3 -c "$probe" >/dev/null 2>&1; then
        command -v python3
    elif command -v sage >/dev/null 2>&1 &&
         sage -python -c "$probe" >/dev/null 2>&1; then
        sage -python -c 'import sys; print(sys.executable)'
    elif command -v mamba >/dev/null 2>&1 &&
         mamba run -n sage python -c "$probe" >/dev/null 2>&1; then
        mamba run -n sage python -c 'import sys; print(sys.executable)'
    else
        return 1
    fi
}

# run_crash <candidate> <report-id> <library> <security-check>
# The listed malformed-input findings intentionally terminate their process.
run_crash() {
    cand=$1; label=$2; lib=$3; check=$4
    [ -n "$only" ] && [ "$only" != "$cand" ] && return 0
    if [ ! -x "$S" ]; then
        echo "SKIP   $cand $label (build it: make -C security)"
        skipped=$((skipped + 1))
        return 0
    fi
    if [ ! -f "$lib" ]; then
        echo "SKIP   $cand $label (build it: make -C $cand)"
        skipped=$((skipped + 1))
        return 0
    fi
    out=$( (ulimit -c 0; "$S" "$lib" "$check") 2>&1)
    rc=$?
    if [ "$rc" -gt 128 ]; then
        echo "$label  EXPECTED PROCESS TERMINATION rc=$rc"
    else
        echo "UNEXPECTED $label did not terminate by signal (rc=$rc): $out"
        fail=$((fail + 1))
    fi
}

echo "== hash-01-2 AFS-TrEDM: final-block replay distinguisher (High) =="
run_target hash-01 "hash-01-2" python3 hash-01/reproduce_final_block_replay.py

echo
echo "== hash-02-1 AXIS: allocation failure falsely reports success (Low) =="
run_target hash-02 "hash-02-1" make -C hash-02 exploit

echo
echo "== hash-02-3 AXIS: invertible state bounds second-preimage security (Critical) =="
run_target hash-02 "hash-02-3" make -C hash-02 reproduce-inverse

echo
echo "== hash-05-3 uHash: partial-byte padding collisions (Critical) =="
run_target hash-05 "hash-05-3" make -C hash-05 reproduce

echo
echo "== hash-10-1 FEILIAN: allocation failure falsely reports success (Low) =="
run_target hash-10 "hash-10-1" make -C hash-10 exploit

echo
echo "== hash-10-2 FEILIAN: RTL length-domain collisions (Critical) =="
if [ -z "$only" ] || [ "$only" = hash-10 ]; then
    if command -v verilator >/dev/null 2>&1; then
        run_target hash-10 "hash-10-2" make -C hash-10 rtl-exploit
    else
        echo "SKIP   hash-10-2 (install Verilator)"
        skipped=$((skipped + 1))
    fi
fi

echo
echo "== hash-10-4 FEILIAN: unused partial-byte bits affect C hashes (Low) =="
run_target hash-10 "hash-10-4" make -C hash-10 unused-bits

echo
echo "== hash-12-1 Iphe: cross-profile output relation (Medium) =="
run_target hash-12 "hash-12-1" make -C hash-12 exploit

echo
echo "== hash-04-1 / hash-04-2 CHAMP: determinant-fiber bound and projective lead (Critical / Low Lead) =="
run_target hash-04 "hash-04-1/hash-04-2" python3 hash-04/reproduce_structure.py

echo "== hash-04-3 / hash-04-4 CHAMP: square-root preimages and full-size writes =="
run_target hash-04 "hash-04-3/hash-04-4" make -C hash-04 reproduce

echo "== hash-09-1 Eijen: trivial collisions (Critical) =="
for l in hash-09/lib/*.so; do
    run hash-09 "hash-09-1" CONFIRMED hash-collide-zeropad "$l"
done
run hash-04 "[control hash-09-1]" NOT-CONFIRMED hash-collide-zeropad hash-04/lib/libCHAMP-512.so
run hash-12 "[control hash-09-1]" NOT-CONFIRMED hash-collide-zeropad hash-12/lib/libIphe-1024.so

echo
echo "== hash-09-2 Eijen: cross-instance digest suffix (Medium) =="
run_target hash-09 "hash-09-2" python3 hash-09/reproduce_cross_instance_suffix.py

echo
echo "== hash-17-1 MasterCube: pad10*1 boundary collisions (Critical) =="
run hash-17 "hash-17-1" CONFIRMED hash-collide-rate hash-17/lib/libMasterCube-512.so  959
run hash-17 "hash-17-1" CONFIRMED hash-collide-rate hash-17/lib/libMasterCube-768.so  703
run hash-17 "hash-17-1" CONFIRMED hash-collide-rate hash-17/lib/libMasterCube-1024.so 447
run hash-17 "[control hash-17-1]" NOT-CONFIRMED hash-collide-rate hash-17/lib/libMasterCube-512.so 955

echo
echo "== hash-18-1 / hash-20-1: no domain separation between digest lengths (Medium) =="
run hash-18 "hash-18-1" CONFIRMED hash-prefix hash-18/lib/libMEGASCON-384.so hash-18/lib/libMEGASCON-512.so
run hash-20 "hash-20-1" CONFIRMED hash-prefix hash-20/lib/libMOZI-384.so     hash-20/lib/libMOZI-512.so
run hash-04 "[control hash-18-1/hash-20-1]" NOT-CONFIRMED hash-prefix hash-04/lib/libCHAMP-512.so hash-04/lib/libCHAMP-1024.so

echo
echo "== hash-14-1 Laurus: function-domain separation loss =="
run_target hash-14 "hash-14-1" make -C hash-14 reproduce

echo
echo "== hash-17-2 MasterCube: inverse-round mismatch =="
run_target hash-17 "hash-17-2" make -C hash-17 reproduce-inverse

echo
echo "== hash-19-1 MoFang: deterministic full-round collisions =="
run_target hash-19 "hash-19-1" make -C hash-19 reproduce

echo
echo "== hash-21-1 / hash-21-2 Neulaser: state-merger collisions =="
run_target hash-21 "hash-21-1/hash-21-2" make -C hash-21 reproduce

echo
echo "== hash-24-1 QSH: invariant-subspace distinguisher =="
run_target hash-24 "hash-24-1" make -C hash-24 reproduce

echo
echo "== hash-24-3 QSH: free-start and semi-free-start collisions (Medium) =="
run_target hash-24 "hash-24-3" make -C hash-24 reproduce-free-start

echo
echo "== hash-25-1 TaiChi: allocation failure falsely reports success (Low) =="
run_target hash-25 "hash-25-1" make -C hash-25 exploit

echo
echo "== hash-26-1 CHIME: invariant-subspace collision bounds =="
run_target hash-26 "hash-26-1" make -C hash-26 reproduce

echo
echo "== hash-31-1 ZC-DMC: cross-domain distinguisher =="
run_target hash-31 "hash-31-1" make -C hash-31 exploit

echo
echo "== hash-30-1 / hash-31-2 / hash-32-2 ZC: conditional iterative trail (High Lead) =="
if [ -z "$only" ] || [ "$only" = hash-30 ] ||
   [ "$only" = hash-31 ] || [ "$only" = hash-32 ]; then
    python3 security/zc_iterative_differential.py || fail=$((fail + 1))
fi

echo
echo "== kem-01-1 Aigis-Enc+: dead implicit rejection (Critical) =="
for l in kem-01/lib/*.so; do
    run kem-01 "kem-01-1" CONFIRMED kem-ct-flip "$l"
done

echo
echo "== kem-02-1 Amoeba-576: chosen-ciphertext full secret-key recovery (Critical) =="
if [ -z "$only" ] || [ "$only" = kem-02 ]; then
    if [ -n "${AMOEBA_PYTHON:-}" ] && "$AMOEBA_PYTHON" -c 'import numpy' >/dev/null 2>&1; then
        run_target kem-02 "kem-02-1" make -C kem-02 exploit-key-recovery PYTHON="$AMOEBA_PYTHON"
    elif [ -n "${NGCC_SAGE_PYTHON:-}" ] && "$NGCC_SAGE_PYTHON" -c 'import numpy' >/dev/null 2>&1; then
        run_target kem-02 "kem-02-1" make -C kem-02 exploit-key-recovery PYTHON="$NGCC_SAGE_PYTHON"
    elif python3 -c 'import numpy' >/dev/null 2>&1; then
        run_target kem-02 "kem-02-1" make -C kem-02 exploit-key-recovery PYTHON=python3
    elif command -v mamba >/dev/null 2>&1 && mamba run -n sage python -c 'import numpy' >/dev/null 2>&1; then
        run_target kem-02 "kem-02-1" mamba run -n sage make -C kem-02 exploit-key-recovery PYTHON=python
    else
        echo "SKIP   kem-02 kem-02-1 (set AMOEBA_PYTHON to a NumPy-enabled Python)"
        skipped=$((skipped + 1))
    fi
fi

echo
echo "== kem-02-3 Amoeba: two-error decryption-failure accounting (Medium) =="
run_target kem-02 "kem-02-3" python3 kem-02/reproduce_dfr_tail.py

echo
echo "== kem-02-4 Amoeba: Hamming correction stack write (High) =="
run_target kem-02 "kem-02-4" python3 kem-02/reproduce_ecc_stack_write.py

echo
echo "== kem-02-6 Amoeba: pre-FO decoder-oracle key recovery (Critical) =="
run_target kem-02 "kem-02-6" make -C kem-02 exploit-dfo-key-recovery

echo
echo "== kem-03-2 BAG-Loong: rejection key omits received ciphertext (High) =="
run_target kem-03 "kem-03-2" python3 kem-03/reproduce_rejection_key.py kem-03/lib/libBAG-Loong-128.so

echo
echo "== kem-03-3 BAG-Loong: public secret-support spaces (Critical) =="
run_target kem-03 "kem-03-3" python3 kem-03/reproduce_public_supports.py
run_target kem-03 "kem-03-3" make -C kem-03 reproduce-public-supports

echo
echo "== kem-04-2 BAG-Piglet: rejection key omits received ciphertext (High) =="
run_target kem-04 "kem-04-2" python3 kem-04/reproduce_rejection_key.py kem-04/lib/libbag_piglet_128.so

echo
echo "== kem-05-1 BIKE-MLThre: unseeded default key generation exposes the secret key (Critical) =="
run_target kem-05 "kem-05-1" python3 kem-05/reproduce_unseeded_drbg.py

echo
echo "== kem-19-1 Lore: ring-projection preflight (Lead; lattice cost unverified) =="
run_target kem-19 "kem-19-1" python3 kem-19/reproduce_ring_projection.py

echo
echo "== kem-22-1 Mithril: honest shared-secret mismatch (Medium) =="
run_target kem-22 "kem-22-1" make -C kem-22 exploit-decoder

echo
echo "== kem-26-1 / kem-26-2 NSS-HQC: parity distinguisher and honest failure (Medium) =="
run_target kem-26 "kem-26-1" python3 kem-26/reproduce_parity.py
run_target kem-26 "kem-26-2" python3 kem-26/reproduce_failure.py

echo
echo "== kem-25-1 NEV: compressed-set rejection mismatch (Medium Proof gap) =="
run_target kem-25 "kem-25-1" python3 kem-25/reproduce_rejection_mismatch.py

echo
echo "== kem-28-1 OAEP-NTRU: noncanonical ciphertext aliases (Critical) =="
run_target kem-28 "kem-28-1" python3 kem-28/reproduce_noncanonical_ciphertext.py

echo
echo "== kem-09-1 / kem-18-1: rejection mask leaks the secret (Critical) =="
for l in kem-09/lib/*.so; do run kem-09 "kem-09-1" CONFIRMED kem-reject-mask "$l"; done
for l in kem-18/lib/*.so; do run kem-18 "kem-18-1" CONFIRMED kem-reject-mask "$l"; done
run kem-22 "[control kem-09-1/kem-18-1]" NOT-CONFIRMED kem-reject-mask kem-22/lib/libMithril-128.so

echo
echo "== kem-18-2 LoongKEM: reducible-ring quotient attacks (High lead) =="
if [ -z "$only" ] || [ "$only" = kem-18 ]; then
    if [ -n "${NGCC_SAGE_PYTHON:-}" ] && "$NGCC_SAGE_PYTHON" -c 'import sage.all' >/dev/null 2>&1; then
        run_target kem-18 "kem-18-2" "$NGCC_SAGE_PYTHON" kem-18/reproduce_reducible_ring.py
    elif command -v sage >/dev/null 2>&1 && sage -python -c 'import sage.all' >/dev/null 2>&1; then
        run_target kem-18 "kem-18-2" sage -python kem-18/reproduce_reducible_ring.py
    elif python3 -c 'import sage.all' >/dev/null 2>&1; then
        run_target kem-18 "kem-18-2" python3 kem-18/reproduce_reducible_ring.py
    elif command -v mamba >/dev/null 2>&1 && mamba run -n sage python -c 'import sage.all' >/dev/null 2>&1; then
        run_target kem-18 "kem-18-2" mamba run -n sage python kem-18/reproduce_reducible_ring.py
    else
        echo "SKIP   kem-18 kem-18-2 (set NGCC_SAGE_PYTHON to a Sage-enabled Python)"
        skipped=$((skipped + 1))
    fi
fi

echo
echo "== kem-18-3 Loong128: public-only shared-secret recovery (Critical; may take minutes) =="
if [ -z "$only" ] || [ "$only" = kem-18 ]; then
    loong_python=${NGCC_FPYLLL_PYTHON:-}
    if [ -z "$loong_python" ] && python3 -c 'import fpylll' >/dev/null 2>&1; then
        loong_python=python3
    elif [ -z "$loong_python" ] && command -v mamba >/dev/null 2>&1 &&
         mamba run -n sage python -c 'import fpylll' >/dev/null 2>&1; then
        loong_python=$(mamba run -n sage which python | tail -n 1)
    fi
    if [ -n "$loong_python" ] && "$loong_python" -c 'import fpylll' >/dev/null 2>&1; then
        run_target kem-18 "kem-18-3" env NGCC_FPYLLL_PYTHON="$loong_python" make -C kem-18 reproduce-public-recovery
    else
        echo "SKIP   kem-18-3 (install Python fpylll or set NGCC_FPYLLL_PYTHON)"
        skipped=$((skipped + 1))
    fi
fi

echo
echo "== kem-06-1 / kem-06-2 BRA: decoder and field out-of-bounds accesses =="
if [ -z "$only" ] || [ "$only" = kem-06 ]; then
    run_target kem-06 "kem-06-1" make -C kem-06 exploit
    run_target kem-06 "kem-06-2" make -C kem-06 exploit-field
    run_target kem-06 "kem-06-1 ciphertext extension" make -C kem-06 exploit-ciphertext-crash
fi

echo
echo "== kem-06-4 BRA: ignored ciphertext padding (Critical) =="
run_target kem-06 "kem-06-4" python3 kem-06/reproduce_padding_alias.py

echo
echo "== kem-07-2 BRQC: ignored ciphertext padding (Critical) =="
run_target kem-07 "kem-07-2" python3 kem-07/reproduce_padding_alias.py

echo
echo "== kem-10-1 C-Multi-UR-AG: malformed-ciphertext process crashes (High) =="
run_crash kem-10 "kem-10-1" kem-10/lib/libCMultiURAG-128.so kem-zero
run_crash kem-10 "kem-10-1" kem-10/lib/libCMultiURAG-256.so kem-ciphertext-flip
run_crash kem-10 "kem-10-1" kem-10/lib/libCMultiURAG-512.so kem-zero

echo
echo "== kem-10-3 C-Multi-UR-AG: ignored ciphertext padding (Critical) =="
run_target kem-10 "kem-10-3" python3 kem-10/reproduce_padding_alias.py

echo
echo "== kem-13-1 DKEM: rejection key omits c1 (Medium) =="
run_target kem-13 "kem-13-1" make -C kem-13 reproduce-rejection-binding

echo
echo "== kem-13-2 DKEM: malicious public key repeats the sender key (Medium) =="
run_target kem-13 "kem-13-2" make -C kem-13 reproduce-malicious-key

echo
echo "== kem-13-3 DKEM: int16 NTT overflow breaks honest DKEM-512 sessions (Medium) =="
run_target kem-13 "kem-13-3" make -C kem-13 reproduce-ntt-overflow

echo
echo "== kem-13-4 DKEM: ignored input lengths permit fixed-size over-reads (Low) =="
run_target kem-13 "kem-13-4" make -C kem-13 reproduce-truncated

echo
echo "== kem-14-1 DTRU: caller length causes stack overflow (High) =="
run_target kem-14 "kem-14-1" make -C kem-14 exploit

echo
echo "== kem-14-2/-3 DTRU: rejection oracle and missing public-key binding (Low) =="
run_target kem-14 "kem-14-2/kem-14-3" make -C kem-14 exploit-rejection-contract

echo
echo "== kem-15-1 FLIT512: reference/optimized interoperability failure (Low) =="
run_target kem-15 "kem-15-1" python3 kem-15/reproduce_interop.py

echo
echo "== kem-16-1 HARE: headline DFR claims require refined model (Medium Proof gap) =="
run_target kem-16 "kem-16-1" python3 kem-16/reproduce_model1_dfr.py

echo
echo "== kem-17-4 HEP-QC: public EPC-P column fingerprint =="
run_target kem-17 "kem-17-4" python3 kem-17/reproduce_epcp_fingerprint.py

echo
echo "== kem-27-1 NTRE: scaled seed-ceiling witness (Critical) =="
run_target kem-27 "kem-27-1" make -C kem-27 exploit

echo
echo "== kem-31-1 QIMEN-PIKE: invalid-ciphertext assertion aborts (Low) =="
run_target kem-31 "kem-31-1" make -C kem-31 reproduce-hint
run_crash kem-31 "kem-31-1" kem-31/lib/libNGCC-1.so kem-zero
run_crash kem-31 "kem-31-1" kem-31/lib/libNGCC-2.so kem-zero
run_crash kem-31 "kem-31-1" kem-31/lib/libNGCC-3.so kem-zero

echo
echo "== kem-31-2 QIMEN-PIKE: non-canonical ciphertext aliases (Critical) =="
run_target kem-31 "kem-31-2" python3 kem-31/reproduce_ciphertext_alias.py

echo
echo "== kem-31-3 QIMEN-PIKE: malformed public-key denial of service (Low) =="
run_target kem-31 "kem-31-3" python3 kem-31/reproduce_malformed_public_key.py --timeout 10

echo
echo "== kem-31-4 QIMEN-PIKE: shared torsion mask leaks a square-coset constraint (Medium Confirmed) =="
run_target kem-31 "kem-31-4" python3 kem-31/reproduce_pairing_constraint.py

echo
echo "== kem-33-1 QUBE: secret sampler has variable work (Medium) =="
run_target kem-33 "kem-33-1" make -C kem-33 exploit

echo
echo "== kem-35-1 Scloud+: decrypted-message-dependent re-encryption timing (Medium) =="
run_target kem-35 "kem-35-1" sh kem-35/reproduce_reencryption_timing.sh

echo
echo "== kem-37-1 TriQ-KEM: secret sampler has variable work (Medium) =="
run_target kem-37 "kem-37-1" make -C kem-37 exploit

echo
echo "== kem-36-1 TRIKE: specified maximum threshold rejects honest ciphertexts (High) =="
if [ -z "$only" ] || [ "$only" = kem-36 ]; then
    if [ -f kem-36/lib/libTRIKE-2.so ]; then
        out=$(python3 security/trike_threshold_differential.py --trials 32 2>&1)
        rc=$?
        case "$out" in
            *'"confirmed": true'*'"shipped_min_failures": 0'*'"specified_max_failures": 32'*)
                [ "$rc" -eq 0 ] && echo "kem-36-1  ATTACK trike-threshold       TRIKE-2 CONFIRMED shipped-min=32/32 specified-max=0/32" || fail=$((fail + 1))
                ;;
            *) echo "UNEXPECTED kem-36-1 output (rc=$rc): $out"; fail=$((fail + 1)) ;;
        esac
    else
        echo "SKIP   kem-36 (build it: make -C kem-36)"; skipped=$((skipped + 1))
    fi
fi

echo
echo "== kem-38-2 UVW: stable list-decoding failure oracle (High) =="
if [ -z "$only" ] || [ "$only" = kem-38 ]; then
    if [ -f kem-38/lib/libUVW-KEM-128.so ]; then
        out=$(python3 security/kem_mutation_oracle.py kem-38/lib/libUVW-KEM-128.so --bits 0,846 2>&1)
        rc=$?
        case "$out" in
            *'"-1": 1'*'"-2": 1'*)
                [ "$rc" -eq 0 ] && echo "kem-38-2  ATTACK kem-failure-oracle     UVW-KEM-128 CONFIRMED mutations expose both -2 decoder and -1 validation failures" || fail=$((fail + 1))
                ;;
            *) echo "UNEXPECTED kem-38-2 output (rc=$rc): $out"; fail=$((fail + 1)) ;;
        esac
    else
        echo "SKIP   kem-38 (build it: make -C kem-38)"; skipped=$((skipped + 1))
    fi
fi

echo
echo "== kem-38-4 UVW: ignored ciphertext bits (Critical) =="
run_target kem-38 "kem-38-4" python3 kem-38/reproduce_padding_alias.py

echo
echo "== kem-29-1 Polar-KEM: public-key-only shared-secret recovery (Critical) =="
if [ -z "$only" ] || [ "$only" = kem-29 ]; then
    if [ -f kem-29/lib/libPolarKEM-128.so ]; then
        python3 kem-29/reproduce_public_recovery.py || fail=$((fail + 1))
    else
        echo "SKIP   kem-29 (build it: make -C kem-29)"; skipped=$((skipped + 1))
    fi
fi

echo
echo "== kem-29-2 Polar-KEM: specified aligned basis exposes secret isometry (Critical) =="
run_target kem-29 "kem-29-2" python3 kem-29/reproduce_spec_alignment.py

echo
echo "== kem-29-3 / kem-29-4 Polar-KEM: radius failure and ciphertext aliases =="
run_target kem-29 "kem-29-3/kem-29-4" make -C kem-29 audit-spec

echo
echo "== kem-30-1 PolarLAC: decrypted-message timing classes (Medium) =="
run_target kem-30 "kem-30-1" make -C kem-30 reproduce-timing-leak

echo
echo "== kex-06-2 MAMBA-NIKE: static-key reaction-recovery path (High lead) =="
run_target kex-06 "kex-06-2" make -C kex-06 exploit-reaction-recovery

echo
echo "== kex-06-3 MAMBA-NIKE: passive-security reconciliation proof gap (Medium) =="
run_target kex-06 "kex-06-3" python3 kex-06/reproduce_reconciliation_bound.py

echo
echo "== kex-02-1 AFS-KEX: completed-session key recovery after long-term compromise (Critical) =="
if [ -z "$only" ] || [ "$only" = kex-02 ]; then
    if [ -x kex-02/reproduce_pfs_break ] &&
       [ -x kex-02/reproduce_pfs_break_c256 ] &&
       [ -x kex-02/reproduce_pfs_break_c512 ]; then
        kex-02/reproduce_pfs_break || fail=$((fail + 1))
        kex-02/reproduce_pfs_break_c256 || fail=$((fail + 1))
        kex-02/reproduce_pfs_break_c512 || fail=$((fail + 1))
    else
        echo "SKIP   kex-02 (build it: make -C kex-02 exploit)"; skipped=$((skipped + 1))
    fi
fi

echo
echo "== kex-05-1 LoomKEX-256: honest exchange aborts at pass 3 (Low) =="
if [ -z "$only" ] || [ "$only" = kex-05 ]; then
    if [ -x kex-05/reproduce_failure ]; then
        witness=$(mktemp)
        out=$(kex-05/reproduce_failure 136129 1 1 "$witness" 0 2>&1)
        rc=$?
        digest=$(sed '/^implementation=/d' "$witness" | sha256sum | awk '{print $1}')
        rm -f "$witness"
        if [ "$rc" -eq 0 ] && [ "$digest" = 609a6a22da51acd82856d7ed8f125b07373d9ac21af7ee663ac9a6604d4f7689 ]; then
            echo "kex-05-1  ATTACK honest-failure       LoomKEX-256 CONFIRMED pass3 returned -3; normalized witness sha256=$digest"
        else
            echo "UNEXPECTED LoomKEX-256 replay rc=$rc digest=$digest: $out"; fail=$((fail + 1))
        fi
    else
        echo "SKIP   kex-05 (build it: make -C kex-05 replay)"; skipped=$((skipped + 1))
    fi
fi

echo
echo "== kex-01-2 / kex-01-3 ADKEX: honest mismatch and truncated-message read (Low) =="
if [ -z "$only" ] || [ "$only" = kex-01 ]; then
    make -C kex-01 reproduce-correctness || fail=$((fail + 1))
    make -C kex-01 reproduce-truncated || fail=$((fail + 1))
    make -C kex-01 clean-reproducers || fail=$((fail + 1))
fi

echo
echo "== kex-03-2 / kex-03-3 CreTAKE: transcript and double-key KEM binding (Critical / High) =="
run_target kex-03 "kex-03-2/kex-03-3" python3 kex-03/reproduce_binding_attacks.py

echo
echo "== kex-04-1 DKEX-512: honest shared-secret mismatch (Low) =="
run_target kex-04 "kex-04-1" python3 kex-04/reproduce_correctness.py

echo
echo "== kex-04-2 DKEX: ignored input lengths permit fixed-size over-reads (Low) =="
run_target kex-04 "kex-04-2" make -C kex-04 reproduce-truncated

echo
echo "== kex-05-2 LoomKEX-256: ephemeral-key reuse recovery (High) =="
if [ -z "$only" ] || [ "$only" = kex-05 ]; then
    if [ -x kex-05/reproduce_state_rollback_key_recovery ]; then
        out=$(kex-05/reproduce_state_rollback_key_recovery 2>&1)
        rc=$?
        case "$out" in
            *"ATTACK kex-05-2"*"CONFIRMED"*"rollback_queries=4532"*"recovered_coefficients=1024"*"honest_passes=4"*"shared_secret_match=yes"*)
                [ "$rc" -eq 0 ] && printf '%s\n' "$out" || { echo "UNEXPECTED kex-05-2 exit status $rc"; fail=$((fail + 1)); }
                ;;
            *) echo "UNEXPECTED kex-05-2 output (rc=$rc): $out"; fail=$((fail + 1)) ;;
        esac
    else
        echo "SKIP   kex-05 kex-05-2 (build it: make -C kex-05 exploit)"; skipped=$((skipped + 1))
    fi
fi

echo
echo "== kex-05-3 Loom: byte-identical Shuttle authentication module (Critical) =="
run_target kex-05 "kex-05-3" python3 kex-05/validate_shuttle_embedding.py

echo
echo "== kex-08-1 NIIKE: raw shared-invariant distinguisher (Critical) =="
if [ -z "$only" ] || [ "$only" = kex-08 ]; then
    if [ -n "${NIIKE_PYTHON:-}" ] && $NIIKE_PYTHON -c 'import sage.all' >/dev/null 2>&1; then
        run_target kex-08 "kex-08-1" make -C kex-08 exploit PYTHON="$NIIKE_PYTHON"
    elif [ -n "${NGCC_SAGE_PYTHON:-}" ] && "$NGCC_SAGE_PYTHON" -c 'import sage.all' >/dev/null 2>&1; then
        run_target kex-08 "kex-08-1" make -C kex-08 exploit PYTHON="$NGCC_SAGE_PYTHON"
    elif command -v sage >/dev/null 2>&1 && sage -python -c 'import sage.all' >/dev/null 2>&1; then
        run_target kex-08 "kex-08-1" make -C kex-08 exploit PYTHON="sage -python"
    elif python3 -c 'import sage.all' >/dev/null 2>&1; then
        run_target kex-08 "kex-08-1" make -C kex-08 exploit PYTHON=python3
    elif command -v mamba >/dev/null 2>&1 && mamba run -n sage python -c 'import sage.all' >/dev/null 2>&1; then
        run_target kex-08 "kex-08-1" make -C kex-08 exploit PYTHON="mamba run -n sage python"
    else
        echo "SKIP   kex-08 kex-08-1 (set NIIKE_PYTHON to a Sage-enabled Python command)"
        skipped=$((skipped + 1))
    fi
fi

echo
echo "== kex-08-2 NIIKE-lv512: two-key secret cycle (Critical) =="
run_target kex-08 "kex-08-2" make -C kex-08 exploit-lv512

echo
echo "== kex-08-3 NIIKE-lv128: crafted peer key forces the shared secret (Medium) =="
run_target kex-08 "kex-08-3" python3 kex-08/reproduce_malicious_peer_key.py

echo
echo "== kex-08-4 NIIKE-lv128: malformed peer key aborts derivation (Low) =="
run_target kex-08 "kex-08-4" python3 kex-08/reproduce_malformed_key_abort.py

echo
echo "== kem-39-2 WeaverKEM-256: omitted BCH correction (Medium) =="
run_target kem-39 "kem-39-2" make -C kem-39 exploit-bch

echo
echo "== kex-09-1 TriQ-KEX: secret sampler has variable work (Medium) =="
run_target kex-09 "kex-09-1" make -C kex-09 exploit

echo
echo "== sign-03-1 CEDRUS+C: adaptive FORS leaf-accumulation forgery (Critical) =="
if [ -z "$only" ] || [ "$only" = sign-03 ]; then
    if [ -x sign-03/reproduce_forgery ] && [ -f sign-03/lib/libCEDRUSC-160f.so ]; then
        sign-03/reproduce_forgery sign-03/lib/libCEDRUSC-160f.so || fail=$((fail + 1))
    else
        echo "SKIP   sign-03 (build it: make -C sign-03 exploit)"; skipped=$((skipped + 1))
    fi
fi

echo
echo "== sign-22-3 Rhyme-SM3: missing doubled-width parity mask (Medium) =="
run_target sign-22 "sign-22-3 static" make -C sign-22 audit-parity
run_target sign-22 "sign-22-3 runtime" make -C sign-22 audit-parity-runtime

echo
echo "== sign-23-1 Shuttle: covariance key recovery and forgery (Critical) =="
run_target sign-23 "sign-23-1" make -C sign-23 reproduce

echo
echo "== sign-24-1 Sigurd: witness recovery and forgery (Critical) =="
run_target sign-24 "sign-24-1" make -C sign-24 reproduce

echo
echo "== sign-04-1 / sign-04-2 CEDRUS-alpha: WOTS truncation and address aliases =="
run_target sign-04 "sign-04-1/sign-04-2" make -C sign-04 exploit

echo
echo "== sign-05-1 Chinith: public-key-only forgery in all 14 sets (Critical) =="
run_target sign-05 "sign-05-1" make -C sign-05 reproduce

echo
echo "== sign-05-3 Chinith: specified opening retry cannot progress (Low) =="
run_target sign-05 "sign-05-3" python3 sign-05/reproduce_open_retry.py

echo
echo "== sign-06-3 COMPASS-SIG: rejected signatures leak heap memory (Low) =="
run_target sign-06 "sign-06-3" make -C sign-06 reproduce-verify-leak

echo
echo "== sign-06-4 COMPASS-SIG: XOF replay duplicates high-level secrets (High) =="
if [ -z "$only" ] || [ "$only" = sign-06 ]; then
    if [ -f sign-06/lib/libCOMPASS-SIG-128.so ] &&
       [ -f sign-06/lib/libCOMPASS-SIG-256.so ] &&
       [ -f sign-06/lib/libCOMPASS-SIG-384.so ] &&
       [ -f sign-06/lib/libCOMPASS-SIG-512.so ]; then
        python3 sign-06/reproduce_xof_replay.py || fail=$((fail + 1))
    else
        echo "SKIP   sign-06 XOF replay (build it: make -C sign-06)"
        skipped=$((skipped + 1))
    fi
fi

echo
echo "== sign-06-5 COMPASS-SIG: narrow challenge sampler shrinks the 384/512 challenge space (Critical) =="
run_target sign-06 "sign-06-5" make -C sign-06 reproduce-challenge-image
run_target sign-06 "sign-06-5" python3 sign-06/reproduce_challenge_image.py

echo
echo "== sign-01-1 / sign-01-2: SUF-CMA malleability and malformed-hint stack write =="
run sign-01 "sign-01-1/sign-01-2" CONFIRMED sig-malleable sign-01/lib/libAigis-sig1.so
echo
echo "== sign-01-3 / sign-01-4: ASan key-length and signer-write witnesses =="
run_target sign-01 "sign-01-3/sign-01-4" make -C sign-01 exploit-memory
echo
echo "== sign-01-5 Aigis-Sig+: challenge signs collapse to one bit (Critical) =="
run_target sign-01 "sign-01-5" python3 sign-01/reproduce_challenge_entropy.py
echo
echo "== sign-01-6 Aigis-Sig+: printed abort bound outside the theorems' hypothesis (Low) =="
run_target sign-01 "sign-01-6" python3 sign-01/reproduce_abort_bound.py
echo "== sign-07-1: SUF-CMA malleability (High) =="
run sign-07 "sign-07-1" CONFIRMED sig-malleable sign-07/lib/libCS-128.so

echo
echo "== sign-07-2 CS: verifier challenge-sign blindness enables universal forgery (Critical) =="
if [ -z "$only" ] || [ "$only" = sign-07 ]; then
    if [ -x sign-07/forgery_CS-128-scaled-tau3 ] && [ -f sign-07/lib/libCS-128-scaled-tau3.so ]; then
        sign-07/forgery_CS-128-scaled-tau3 sign-07/lib/libCS-128-scaled-tau3.so --threads 4 || fail=$((fail + 1))
        for i in CS-128 CS-256 CS-512; do
            sign-07/forgery_$i sign-07/lib/lib$i.so --control --threads 4 --trials 200000 || fail=$((fail + 1))
        done
    else
        echo "SKIP   sign-07 sign-07-2 (build it: make -C sign-07 exploit)"; skipped=$((skipped + 1))
    fi
fi

echo
echo "== sign-08-2 DARTS-128: compression restart leaks the signing key (Critical) =="
run_target sign-08 "sign-08-2" make -C sign-08 exploit-key-recovery

echo
echo "== sign-10-1 Facto-DSA: hidden-zero-subspace algebraic recovery lead (High) =="
if [ -z "$only" ] || [ "$only" = sign-10 ]; then
    # This directory is intentionally ignored. Rebuild so an executable left
    # by another x86-64 host cannot turn the check into a SIGILL.
    make -C sign-10/cryptanalysis clean >/dev/null
    make -C sign-10/cryptanalysis test || fail=$((fail + 1))
fi

echo
echo "== sign-10-2 Facto-DSA: public-key universal signing trapdoor =="
run_target sign-10 "sign-10-2" make -C sign-10 reproduce-forgery

echo
echo "== sign-13-1 GreatWall-512: SHAKE256 capacity forgery bound (Critical) =="
run_target sign-13 "sign-13-1" python3 sign-13/reproduce_shake_capacity.py

echo
echo "== sign-14-1 Lynxer: universal public-key-only forgery (Critical) =="
run_target sign-14 "sign-14-1" make -C sign-14 exploit-public-forgery

echo
echo "== sign-11-5 FlexTree: unchecked PORS padding is malleable (Medium) =="
run sign-11 "sign-11-5" CONFIRMED sig-pors-padding sign-11/lib/libFlextree-160f.so

echo
echo "== sign-09-1 DOVE: unauthenticated salt (Medium) =="
run_target sign-09 "sign-09-1" make -C sign-09 exploit

echo
echo "== sign-15-2 MORNING-ATLAS: ignored hint padding violates SUF-CMA (High) =="
for l in sign-15/lib/*.so; do
    run sign-15 "sign-15-2" CONFIRMED sig-hint-padding "$l"
done

echo
echo "== sign-15-4 MORNING-ATLAS: repeated-mask key recovery and forgery (Critical) =="
if [ -z "$only" ] || [ "$only" = sign-15 ]; then
    if [ -n "${NGCC_SAGE_PYTHON:-}" ]; then
        "$NGCC_SAGE_PYTHON" sign-15/reproduce_mask_key_recovery.py || fail=$((fail + 1))
    elif command -v sage >/dev/null 2>&1 && sage -python -c 'import sage.all' >/dev/null 2>&1; then
        sage -python sign-15/reproduce_mask_key_recovery.py || fail=$((fail + 1))
    elif python3 -c 'import sage.all' >/dev/null 2>&1; then
        python3 sign-15/reproduce_mask_key_recovery.py || fail=$((fail + 1))
    elif command -v mamba >/dev/null 2>&1 &&
         mamba run -n sage python -c 'import sage.all' >/dev/null 2>&1; then
        mamba run -n sage python sign-15/reproduce_mask_key_recovery.py || fail=$((fail + 1))
    else
        echo "SKIP   sign-15 sign-15-4 (set NGCC_SAGE_PYTHON to a Sage-enabled Python)"
        skipped=$((skipped + 1))
    fi
fi

echo
echo "== sign-15-5 MORNING-ATLAS: malformed-hint memory errors =="
run_target sign-15 "sign-15-5" make -C sign-15 exploit-memory-safety

echo
echo "== sign-15-7 MORNING-ATLAS: specified finite-support key recovery (Critical) =="
run_target sign-15 "sign-15-7" make -C sign-15 exploit-spec-key-recovery

echo
echo "== sign-25-1 SQIsign2D2: verifier verdict depends on stale stack state (Critical) =="
run sign-25 "sign-25-1" CONFIRMED sig-uninit-verdict sign-25/lib/libSQISign2Dsquare-Level2-eff_uncompressed.so
run sign-25 "[control sign-25-1]" NOT-CONFIRMED sig-uninit-verdict sign-25/lib/libSQISign2Dsquare-Level2-eff_compressed.so

echo
echo "== sign-25-3 SQIsign2D2: compact challenge retargeting (Critical) =="
run_target sign-25 "sign-25-3" make -C sign-25 exploit-compact-retarget

echo
echo "== sign-27-1 SQIsignTriangle: all-zero signature aborts (Low) =="
run_crash sign-27 "sign-27-1" sign-27/lib/libSQIsignTriangle_lvl1.so sig-zero

echo
echo "== sign-27-3/-4 SQIsignTriangle: modular challenge transfer and proof failure =="
run_target sign-27 "sign-27-3/sign-27-4" make -C sign-27 exploit-modular-challenge

echo
echo "== sign-27-5 SQIsignTriangle: response-rescaling fresh-message forgery (Critical) =="
run_target sign-27 "sign-27-5" make -C sign-27 exploit-response-rescaling

echo
echo "== sign-18-2 Origami: signature constraint-subspace recovery =="
run_target sign-18 "sign-18-2" python3 sign-18/reproduce_signature_subspace.py

echo
echo "== sign-18-5 Origami: public-key-only signature forgery (Critical) =="
run_target sign-18 "sign-18-5" python3 sign-18/reproduce_public_forgery.py

echo
echo "== sign-20-1 Qingluan-128: quantum-accounting proof gap (Medium) =="
run_target sign-20 "sign-20-1" python3 sign-20/reproduce_quantum_accounting.py

echo
echo "== sign-21-1 ReSolveD-alpha: shared-tweak multi-target key recovery (Critical) =="
run_target sign-21 "sign-21-1" sh sign-21/reproduce_tccr_multitarget.sh

echo
echo "== sign-21-2 ReSolveD-alpha: fixed-tweak leaf multi-target recovery (Critical) =="
run_target sign-21 "sign-21-2" python3 sign-21/reproduce_leaf_commitment_multitarget.py

echo
echo "== sign-21-3 ReSolveD-alpha: deterministic cross-profile witness recovery (High) =="
run_target sign-21 "sign-21-3" sh sign-21/reproduce_cross_profile_recovery.sh

echo
echo "== sign-22-4 Rhyme-128: order-dependent secret-tail recovery and forgery (Critical) =="
run_target sign-22 "sign-22-4" sh sign-22/reproduce_order_dependent_forgery.sh

echo
echo "== sign-26-2 SQIsign2D-push: missing challenge grinding (Critical) =="
run_target sign-26 "sign-26-2" python3 sign-26/reproduce_grinding_shortfall.py

echo
echo "== sign-28-1 SYDO: grinding deficit (Critical) =="
run_target sign-28 "sign-28-1" sh sign-28/reproduce_forum_findings.sh

echo
echo "== sign-28-2 SYDO: padding malleability (Medium; slow) =="
if [ "$only" = sign-28 ] || [ "${NGCC_SLOW:-0}" = 1 ]; then
    run_target sign-28 "sign-28-2" env SETS=160f sh sign-28/reproduce_forum_findings.sh full
elif [ -z "$only" ]; then
    echo "SKIP   sign-28 sign-28-2 full replay (run tools/reproduce.sh sign-28 or set NGCC_SLOW=1)"
    skipped=$((skipped + 1))
fi

echo
echo "== sign-28-3 / sign-28-4 SYDO: Hash4 mismatch and stack over-read (Medium / Low) =="
run_target sign-28 "sign-28-3/sign-28-4" python3 sign-28/reproduce_static_findings.py

echo
echo "== sign-28-5 SYDO: undocumented helper uses a public RNG seed (Medium) =="
run_target sign-28 "sign-28-5" make -C sign-28 reproduce-fixed-rng

echo
echo "== sign-19-1 Phoenix: reference/AVX2 signature interoperability failure (Low) =="
run_target sign-19 "sign-19-1" python3 sign-19/reproduce_interop.py

echo
echo "== sign-29-1 Tins: one-signature witness recovery =="
run_target sign-29 "sign-29-1" make -C sign-29 exploit

echo
echo "== sign-30-1 TRINE: unseeded normal build exposes the signing key (Critical) =="
run_target sign-30 "sign-30-1" python3 sign-30/reproduce_unseeded_forgery.py

echo
echo "== sign-02-2 BiT-128: shared-sign equivalent-key recovery (Critical Confirmed) =="
if [ -z "$only" ] && [ "${NGCC_SLOW:-0}" != 1 ]; then
    echo "SKIP   sign-02 sign-02-2 (set NGCC_SLOW=1 or request sign-02; about 3 minutes)"
    skipped=$((skipped + 1))
else
    run_target sign-02 "sign-02-2" sh sign-02/reproduce_bimodal_key_recovery.sh
fi

echo
echo "== sign-32-1 / sign-32-2 UVW: universal acceptance and verifier crashes =="
run sign-32 "sign-32-1/sign-32-2" CONFIRMED sig-accept-all sign-32/lib/libUVW-128.so
run sign-32 "sign-32-1/sign-32-2" CONFIRMED sig-accept-all sign-32/lib/libUVW-256.so

echo
echo "== sign-32-3 UVW-128: pair-leakage equivalent-key recovery (Critical Confirmed) =="
run_target sign-32 "sign-32-3" sh sign-32/reproduce_pair_leakage_forgery.sh

echo
echo "== sign-12-1 Galas: key generation ignores the seed (Critical) =="
run sign-12 "sign-12-1" CONFIRMED keygen-determinism sign-12/lib/libGalas-160S.so
run kem-01  "[control sign-12-1]" NOT-CONFIRMED keygen-determinism kem-01/lib/libAigis-enc1.so

echo
echo "== kem-17-1 / sign-33-1: identical key in every fresh process (Critical) =="
if { [ -z "$only" ] || [ "$only" = kem-17 ]; } && [ -f kem-17/lib/libhep-qc-1.so ]; then
    a=$("$A" keygen-fresh kem-17/lib/libhep-qc-1.so 0x01 | awk '{print $NF}')
    b=$("$A" keygen-fresh kem-17/lib/libhep-qc-1.so 0x99 | awk '{print $NF}')
    c=$("$A" keygen-fresh kem-01/lib/libAigis-enc1.so 0x01 | awk '{print $NF}')
    d=$("$A" keygen-fresh kem-01/lib/libAigis-enc1.so 0x99 | awk '{print $NF}')
    if [ "$a" = "$b" ]; then
        echo "kem-17-1  ATTACK keygen-fresh         hep-qc-1 CONFIRMED identical first key across two fresh processes with different seeds ($a)"
    else
        echo "UNEXPECTED hep-qc-1 keys differ across seeds"; fail=$((fail + 1))
    fi
    if [ "$c" != "$d" ]; then
        echo "[control kem-17-1] ATTACK keygen-fresh Aigis-enc1 NOT-CONFIRMED keys differ across seeds, as they should"
    else
        echo "UNEXPECTED control Aigis-enc1 keys identical"; fail=$((fail + 1))
    fi
elif [ -z "$only" ] || [ "$only" = kem-17 ]; then
    echo "SKIP   kem-17 (build it: make -C kem-17)"; skipped=$((skipped + 1))
fi

echo
echo "== kem-17-3 HEP-QC: first encapsulation repeats in every fresh process (Critical) =="
if { [ -z "$only" ] || [ "$only" = kem-17 ]; } && [ -f kem-17/lib/libhep-qc-1.so ]; then
    a=$("$A" kem-enc-fresh kem-17/lib/libhep-qc-1.so 0x01 | cut -d' ' -f4-)
    b=$("$A" kem-enc-fresh kem-17/lib/libhep-qc-1.so 0x99 | cut -d' ' -f4-)
    c=$("$A" kem-enc-fresh kem-01/lib/libAigis-enc1.so 0x01 | cut -d' ' -f4-)
    d=$("$A" kem-enc-fresh kem-01/lib/libAigis-enc1.so 0x99 | cut -d' ' -f4-)
    if [ "$a" = "$b" ]; then
        echo "kem-17-3  ATTACK kem-enc-fresh        hep-qc-1 CONFIRMED identical first pk, ciphertext and secret across different seeds ($a)"
    else
        echo "UNEXPECTED hep-qc-1 first encapsulations differ across seeds"; fail=$((fail + 1))
    fi
    if [ "$c" != "$d" ]; then
        echo "[control kem-17-3] ATTACK kem-enc-fresh Aigis-enc1 NOT-CONFIRMED first encapsulations differ across seeds, as they should"
    else
        echo "UNEXPECTED control Aigis-enc1 first encapsulations identical"; fail=$((fail + 1))
    fi
elif [ -z "$only" ] || [ "$only" = kem-17 ]; then
    echo "SKIP   kem-17 (build it: make -C kem-17)"; skipped=$((skipped + 1))
fi

# VDOO advances an unseeded counter within a process, so successive in-process
# keys differ; the defect shows as an identical FIRST key per fresh process.
if { [ -z "$only" ] || [ "$only" = sign-33 ]; } && [ -f sign-33/lib/libvdoo_128.so ]; then
    a=$("$A" keygen-fresh sign-33/lib/libvdoo_128.so 0x01 | awk '{print $NF}')
    b=$("$A" keygen-fresh sign-33/lib/libvdoo_128.so 0x99 | awk '{print $NF}')
    if [ "$a" = "$b" ]; then
        echo "sign-33-1 ATTACK keygen-fresh         VDOO-128 CONFIRMED identical first key across two fresh processes with different seeds ($a)"
    else
        echo "UNEXPECTED VDOO-128 keys differ across seeds"; fail=$((fail + 1))
    fi
elif [ -z "$only" ] || [ "$only" = sign-33 ]; then
    echo "SKIP   sign-33 (build it: make -C sign-33)"; skipped=$((skipped + 1))
fi

echo
echo "== sign-33-4 VDOO: signing salt repeats across fresh processes and messages (High) =="
if { [ -z "$only" ] || [ "$only" = sign-33 ]; } && [ -f sign-33/lib/libvdoo_128.so ]; then
    a=$("$A" sig-random-fresh sign-33/lib/libvdoo_128.so 0x01 0x10 | awk '{print $5, $6}')
    b=$("$A" sig-random-fresh sign-33/lib/libvdoo_128.so 0x99 0x12 | awk '{print $5, $6}')
    if [ "$a" = "$b" ]; then
        echo "sign-33-4 ATTACK sig-random-fresh     VDOO-128 CONFIRMED identical key and salt across different seeds and messages ($a)"
    else
        echo "UNEXPECTED VDOO-128 fresh-process key or salt differs"; fail=$((fail + 1))
    fi
elif [ -z "$only" ] || [ "$only" = sign-33 ]; then
    echo "SKIP   sign-33 (build it: make -C sign-33)"; skipped=$((skipped + 1))
fi

echo
echo "== sign-33-6 VDOO: public-key-only structural forgery (Critical Confirmed) =="
if [ "${NGCC_SLOW:-0}" = 1 ]; then
    run_target sign-33 "sign-33-6" env VDOO_FULL=1 sh sign-33/reproduce_public_structural_forgery.sh
else
    run_target sign-33 "sign-33-6" sh sign-33/reproduce_public_structural_forgery.sh
fi

echo
echo "== sign-34-1 / sign-34-2 YuanYang.DSA: transcript leakage and public-key aliases =="
if [ -z "$only" ] || [ "$only" = sign-34 ]; then
    if [ -x sign-34/reproduce_transcript_leak ] && [ -f sign-34/lib/libyuanyang-512.so ]; then
        sign-34/reproduce_transcript_leak sign-34/lib/libyuanyang-512.so 4000 || fail=$((fail + 1))
    else
        echo "SKIP   sign-34 (build it: make -C sign-34 exploit)"; skipped=$((skipped + 1))
    fi
fi

echo
echo "== kem-05-2 BIKE-MLThre: same-key multi-instance ISD estimates (Critical) =="
if [ -z "$only" ] || [ "$only" = kem-05 ]; then
    estimator_python=${NGCC_SAGE_PYTHON:-}
    if [ -z "$estimator_python" ] &&
       python3 -c 'import numpy, scipy' >/dev/null 2>&1; then
        estimator_python=python3
    elif [ -z "$estimator_python" ] && command -v sage >/dev/null 2>&1 &&
         sage -python -c 'import numpy, scipy' >/dev/null 2>&1; then
        estimator_python=$(sage -python -c 'import sys; print(sys.executable)')
    elif [ -z "$estimator_python" ] && command -v mamba >/dev/null 2>&1 &&
         mamba run -n sage python -c 'import numpy, scipy' >/dev/null 2>&1; then
        estimator_python=$(mamba run -n sage python -c 'import sys; print(sys.executable)')
    fi
    if [ -n "$estimator_python" ]; then
        run_target kem-05 "kem-05-2" "$estimator_python" kem-05/reproduce_multi_instance.py
    else
        echo "SKIP   kem-05 kem-05-2 (set NGCC_SAGE_PYTHON to Python with NumPy and SciPy)"
        skipped=$((skipped + 1))
    fi
fi

echo
echo "== kem-18-3 LoongKEM: Loong128 and Loong256 public recovery (Critical) =="
if [ -z "$only" ] || [ "$only" = kem-18 ]; then
    loong_python=${NGCC_FPYLLL_PYTHON:-}
    if [ -z "$loong_python" ] && python3 -c 'import fpylll' >/dev/null 2>&1; then
        loong_python=python3
    elif [ -z "$loong_python" ] && command -v mamba >/dev/null 2>&1 &&
         mamba run -n sage python -c 'import fpylll' >/dev/null 2>&1; then
        loong_python=$(mamba run -n sage which python | tail -n 1)
    fi
    if [ -n "$loong_python" ] && "$loong_python" -c 'import fpylll' >/dev/null 2>&1; then
        run_target kem-18 "kem-18-3 Loong256" env PYTHON="$loong_python" sh kem-18/reproduce_loong256.sh
    else
        echo "SKIP   kem-18-3 Loong256 (install Python fpylll or set NGCC_FPYLLL_PYTHON)"
        skipped=$((skipped + 1))
    fi
fi

echo
echo "== kem-21-1 Viper: 256-bit secret-seed ceiling (Critical) =="
run_target kem-21 "kem-21-1" python3 kem-21/reproduce_seed_ceiling.py

echo
echo "== kem-26-3 NSS-HQC: ephemeral ISD estimates (Critical) =="
if [ -z "$only" ] || [ "$only" = kem-26 ]; then
    estimator_python=$(find_python 'import importlib.metadata; assert importlib.metadata.version("cryptographic-estimators") == "2.1.1"')
    if [ -n "$estimator_python" ]; then
        run_target kem-26 "kem-26-3" "$estimator_python" kem-26/reproduce_isd_estimate.py
    else
        echo "SKIP   kem-26 kem-26-3 (set NGCC_ESTIMATOR_PYTHON to Python with cryptographic-estimators==2.1.1)"
        skipped=$((skipped + 1))
    fi
fi

echo
echo "== kem-32-2 QCTM: same-key multi-instance ISD estimates (Critical) =="
if [ -z "$only" ] || [ "$only" = kem-32 ]; then
    estimator_python=$(find_python 'import numpy, scipy')
    if [ -n "$estimator_python" ]; then
        run_target kem-32 "kem-32-2" "$estimator_python" kem-32/reproduce_multi_instance.py
    else
        echo "SKIP   kem-32 kem-32-2 (set NGCC_ESTIMATOR_PYTHON to Python with NumPy and SciPy)"
        skipped=$((skipped + 1))
    fi
fi

echo
echo "== kem-34-1 / kem-34-2 / kem-34-3 / kem-34-4 Rudraksh2 findings (Medium / Low) =="
run_target kem-34 "kem-34-1" make -C kem-34 reproduce-message-space
run_target kem-34 "kem-34-2/kem-34-4" make -C kem-34 reproduce-parameters
run_target kem-34 "kem-34-3" make -C kem-34 reproduce-length-read

echo
echo "== kem-36-2 / kem-36-3 / kem-36-4 / kem-36-5 / kem-36-6 TRIKE findings =="
run_target kem-36 "kem-36-2" python3 kem-36/reproduce_weak_key_test.py
run_target kem-36 "kem-36-3/kem-36-6" python3 kem-36/reproduce_implementation_issues.py
run_target kem-36 "kem-36-4/kem-36-5" python3 kem-36/reproduce_spec_proof_gaps.py

echo
echo "== kem-37-2 / kex-09-2 re-encryption sampler timing leads (Medium) =="
run_target kem-37 "kem-37-2" python3 kem-37/reproduce_reencrypt_sampler.py
run_target kex-09 "kex-09-2" python3 kex-09/reproduce_reencrypt_sampler.py

echo
echo "== kem-38-5 / kem-38-6 UVW failure reaction and retry-matrix findings =="
run_target kem-38 "kem-38-5/kem-38-6" python3 kem-38/reproduce_dfr_reaction.py

echo
echo "== kem-39-3 / kem-39-4 / kem-39-5 WeaverKEM implementation findings =="
run_target kem-39 "kem-39-3" sh kem-39/reproduce_bch_decoder.sh
run_target kem-39 "kem-39-4/kem-39-5" python3 kem-39/reproduce_spec_mismatches.py

echo
echo "== kem-40-1 YuanYang.KEM: encryption discards the specified error (Medium) =="
run_target kem-40 "kem-40-1" python3 kem-40/reproduce_unused_error.py

echo
echo "== kex-02-3 / kex-02-4 / kex-02-5 AFS-KEX protocol findings =="
run_target kex-02 "kex-02-3/kex-02-4/kex-02-5" make -C kex-02 reproduce-protocol-findings

echo
echo "== sign-05-4 / sign-05-5 Chinith constraint-system findings =="
run_target sign-05 "sign-05-4/sign-05-5" sh sign-05/reproduce_em_constraints.sh

echo
echo "== sign-12-2 Galas: 32-bit message-length truncation (Critical) =="
run_target sign-12 "sign-12-2" make -C sign-12 reproduce-long-message

echo
echo "== sign-12-3 Galas: same-key cross-variant key recovery (High) =="
run_target sign-12 "sign-12-3" sh sign-12/reproduce_cross_variant_key_recovery.sh

echo
echo "== sign-17-1 / sign-17-2 OPS specification findings =="
run_target sign-17 "sign-17-1/sign-17-2" make -C sign-17 reproduce-spec-findings

echo
echo "== sign-34-3 / sign-34-4 YuanYang.DSA sampler findings =="
run_target sign-34 "sign-34-4" python3 sign-34/reproduce_sampler_constants.py
run_target sign-34 "sign-34-3" sh sign-34/reproduce_sampler_mean.sh

echo
if [ "$fail" -eq 0 ] && [ "$skipped" -eq 0 ]; then
    echo "all reproducers behaved as reported"
else
    echo "$fail reproducer(s) did NOT behave as reported ($skipped skipped)"
fi
exit $((fail > 0 || skipped > 0))
