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
    if "$@"; then
        echo "$label  REPRODUCER PASS"
    else
        echo "UNEXPECTED $label reproducer failure"
        fail=$((fail + 1))
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

echo "== hash-02-1 AXIS: allocation failure falsely reports success (Low) =="
run_target hash-02 "hash-02-1" make -C hash-02 exploit

echo
echo "== hash-10-1 FEILIAN: allocation failure falsely reports success (Low) =="
run_target hash-10 "hash-10-1" make -C hash-10 exploit

echo
echo "== hash-12-1 Iphe: cross-profile output relation (Medium) =="
run_target hash-12 "hash-12-1" make -C hash-12 exploit

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
echo "== hash-18-1 / hash-20-1: no domain separation between digest lengths (High) =="
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
echo "== hash-25-1 TaiChi: allocation failure falsely reports success (Low) =="
run_target hash-25 "hash-25-1" make -C hash-25 exploit

echo
echo "== hash-26-1 CHIME: invariant-subspace collision bounds =="
run_target hash-26 "hash-26-1" make -C hash-26 reproduce

echo
echo "== hash-31-1 ZC-DMC: cross-domain distinguisher =="
run_target hash-31 "hash-31-1" make -C hash-31 exploit

echo
echo "== kem-01-1 Aigis-Enc+: dead implicit rejection (Critical) =="
for l in kem-01/lib/*.so; do
    run kem-01 "kem-01-1" CONFIRMED kem-ct-flip "$l"
done

echo
echo "== kem-02-1 Amoeba-576: chosen-ciphertext full secret-key recovery (Critical) =="
run_target kem-02 "kem-02-1" make -C kem-02 exploit-key-recovery \
    PYTHON="${AMOEBA_PYTHON:-python3}"

echo
echo "== kem-09-1 / kem-18-1: rejection mask leaks the secret (Critical) =="
for l in kem-09/lib/*.so; do run kem-09 "kem-09-1" CONFIRMED kem-reject-mask "$l"; done
for l in kem-18/lib/*.so; do run kem-18 "kem-18-1" CONFIRMED kem-reject-mask "$l"; done
run kem-22 "[control kem-09-1/kem-18-1]" NOT-CONFIRMED kem-reject-mask kem-22/lib/libMithril-128.so

echo
echo "== kem-06-1 / kem-06-2 BRA: decoder and field out-of-bounds accesses =="
if [ -z "$only" ] || [ "$only" = kem-06 ]; then
    run_target kem-06 "kem-06-1" make -C kem-06 exploit
    run_target kem-06 "kem-06-2" make -C kem-06 exploit-field
fi

echo
echo "== kem-10-1 C-Multi-UR-AG: malformed-ciphertext process crashes (High) =="
run_crash kem-10 "kem-10-1" kem-10/lib/libCMultiURAG-128.so kem-zero
run_crash kem-10 "kem-10-1" kem-10/lib/libCMultiURAG-256.so kem-ciphertext-flip
run_crash kem-10 "kem-10-1" kem-10/lib/libCMultiURAG-512.so kem-zero

echo
echo "== kem-14-1 DTRU: caller length causes stack overflow (High) =="
run_target kem-14 "kem-14-1" make -C kem-14 exploit

echo
echo "== kem-17-4 HEP-QC: public EPC-P column fingerprint =="
run_target kem-17 "kem-17-4" python3 kem-17/reproduce_epcp_fingerprint.py

echo
echo "== kem-27-1 NTRE: scaled seed-ceiling witness (High) =="
run_target kem-27 "kem-27-1" make -C kem-27 exploit

echo
echo "== kem-31-1 QIMEN-PIKE: invalid-ciphertext assertion aborts (Medium) =="
run_crash kem-31 "kem-31-1" kem-31/lib/libNGCC-1.so kem-zero
run_crash kem-31 "kem-31-1" kem-31/lib/libNGCC-2.so kem-zero
run_crash kem-31 "kem-31-1" kem-31/lib/libNGCC-3.so kem-zero

echo
echo "== kem-33-1 QUBE: secret sampler has variable work (Medium) =="
run_target kem-33 "kem-33-1" make -C kem-33 exploit

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
echo "== kem-38-2 UVW: stable list-decoding failure oracle (Medium) =="
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
echo "== kem-29-1 Polar-KEM: public-key-only shared-secret recovery (Critical) =="
if [ -z "$only" ] || [ "$only" = kem-29 ]; then
    if [ -f kem-29/lib/libPolarKEM-128.so ]; then
        python3 kem-29/reproduce_public_recovery.py || fail=$((fail + 1))
    else
        echo "SKIP   kem-29 (build it: make -C kem-29)"; skipped=$((skipped + 1))
    fi
fi

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
echo "== kex-05-1 LoomKEX-256: honest exchange aborts at pass 3 (High) =="
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
echo "== kex-05-2 LoomKEX-256: rollback-state ephemeral-key recovery (High) =="
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
echo "== kex-08-1 NIIKE: raw shared-invariant distinguisher (High) =="
run_target kex-08 "kex-08-1" make -C kex-08 exploit PYTHON="${NIIKE_PYTHON:-sage -python}"

echo
echo "== kex-08-2 NIIKE-lv512: two-key secret cycle (Critical) =="
run_target kex-08 "kex-08-2" make -C kex-08 exploit-lv512

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
echo "== sign-04-1 / sign-04-2 CEDRUS-alpha: WOTS truncation and address aliases =="
run_target sign-04 "sign-04-1/sign-04-2" make -C sign-04 exploit

echo
echo "== sign-01-1 / sign-01-2: SUF-CMA malleability and malformed-hint stack write =="
run sign-01 "sign-01-1/sign-01-2" CONFIRMED sig-malleable sign-01/lib/libAigis-sig1.so
echo "== sign-07-1: SUF-CMA malleability (High) =="
run sign-07 "sign-07-1" CONFIRMED sig-malleable sign-07/lib/libCS-128.so

echo
echo "== sign-07-2 CS: verifier challenge-sign blindness enables universal forgery (High) =="
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
echo "== sign-10-1 Facto-DSA: hidden-zero-subspace algebraic recovery lead (High) =="
if [ -z "$only" ] || [ "$only" = sign-10 ]; then
    if [ -x sign-10/cryptanalysis/build/selftest ] &&
       [ -x sign-10/cryptanalysis/build/attack1 ] &&
       [ -x sign-10/cryptanalysis/build/attack3 ]; then
        make -C sign-10/cryptanalysis test || fail=$((fail + 1))
    else
        echo "SKIP   sign-10 (build it: make -C sign-10/cryptanalysis)"
        skipped=$((skipped + 1))
    fi
fi

echo
echo "== sign-10-2 Facto-DSA: public-key universal signing trapdoor =="
run_target sign-10 "sign-10-2" make -C sign-10 reproduce-forgery

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
echo "== sign-25-1 SQIsign2D2: verifier verdict depends on stale stack state (Critical) =="
run sign-25 "sign-25-1" CONFIRMED sig-uninit-verdict sign-25/lib/libSQISign2Dsquare-Level2-eff_uncompressed.so
run sign-25 "[control sign-25-1]" NOT-CONFIRMED sig-uninit-verdict sign-25/lib/libSQISign2Dsquare-Level2-eff_compressed.so

echo
echo "== sign-27-1 SQIsignTriangle: all-zero signature aborts (Medium) =="
run_crash sign-27 "sign-27-1" sign-27/lib/libSQIsignTriangle_lvl1.so sig-zero

echo
echo "== sign-18-2 Origami: signature constraint-subspace recovery =="
run_target sign-18 "sign-18-2" python3 sign-18/reproduce_signature_subspace.py

echo
echo "== sign-29-1 Tins: one-signature witness recovery =="
run_target sign-29 "sign-29-1" make -C sign-29 exploit

echo
echo "== sign-32-1 / sign-32-2 UVW: universal acceptance and verifier crashes =="
run sign-32 "sign-32-1/sign-32-2" CONFIRMED sig-accept-all sign-32/lib/libUVW-128.so
run sign-32 "sign-32-1/sign-32-2" CONFIRMED sig-accept-all sign-32/lib/libUVW-256.so

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
echo "== sign-33-4 VDOO: signing salt repeats across fresh processes and messages (Critical) =="
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
echo "== sign-34-1 / sign-34-2 YuanYang.DSA: transcript leakage and public-key aliases =="
if [ -z "$only" ] || [ "$only" = sign-34 ]; then
    if [ -x sign-34/reproduce_transcript_leak ] && [ -f sign-34/lib/libyuanyang-512.so ]; then
        sign-34/reproduce_transcript_leak sign-34/lib/libyuanyang-512.so 4000 || fail=$((fail + 1))
    else
        echo "SKIP   sign-34 (build it: make -C sign-34 exploit)"; skipped=$((skipped + 1))
    fi
fi

echo
if [ "$fail" -eq 0 ] && [ "$skipped" -eq 0 ]; then
    echo "all reproducers behaved as reported"
else
    echo "$fail reproducer(s) did NOT behave as reported ($skipped skipped)"
fi
exit $((fail > 0 || skipped > 0))
