#!/usr/bin/env python3
"""MAYO-5: Lemma 2, Game 4 to Game 5, uses an average-case bound as an identity.

Step 6 of the proof of Lemma 2 (MAYO specification, physical PDF page 30,
printed page 29) says

    "Game5, conditioned on Good v happening, is identical to Game4"

and derives Pr[Game5() = 1] >= Pr[Game4() = 1] (1 - Qs B).  Game 4 draws the
vinegar tuple v uniformly among the tuples for which P*(v + .) + L(Lambda, .)
has full rank; Game 5 draws it uniformly among all tuples.  Let
gamma(P, O, Lambda) be the fraction of vinegar tuples that are good.  Given
the key and the Lambda_i, the event Good v has probability
prod_i gamma(P, O, Lambda_i) in Game 5, so conditioning on it reweights the
key and the Lambda_i by that product.  The conditioned Game 5 is Game 4 only
if gamma is the same for every key and every Lambda.  Lemma 1 (physical page
28) bounds the average of 1 - gamma over (O, P, v) by B; it says nothing about
a single key.

This script

  1. checks the quoted statements in the pinned specification (pdftotext);
  2. computes gamma exactly, by enumeration of all 2^10 vinegar tuples, for a
     toy MAYO over GF(2) with m = 4, o = 3, k = 2, n - o = 5 on the review's
     sample of 14,000 keys times 4 Lambda (seeds 1000..1019), and shows that
     gamma varies with Lambda for a fixed key and, averaged over all Lambda,
     between keys;
  3. control: computes the average-case quantity that Lemma 1 bounds exactly,
     E[1 - gamma] = 1816553/8388608 = 0.21655 <= B = 3/8, and checks that the
     sample average matches it (within 4 standard errors; the differences are
     printed).  The average behaves as Lemma 1 says; the single values do not
     agree with one another.

The toy follows Section 2.1.1 and Algorithm 7 of the specification except for
the field size (q = 2 instead of 16): f(z) = z^4 + z + 1 is irreducible of
degree m, Z = [[1, z], [z, z^3]] is a symmetric MDS matrix with linearly
independent upper entries, and k < n - o, ko >= m, k(k+1)/2 <= m, n - o >= m.
The fast enumeration (tables of the 67 subspaces of GF(2)^4) is checked
against a direct rank computation of the matrix A of Algorithm 7 and against
brute-force evaluation of P*(v + o) + L(Lambda, v + o) of Definition 2 over
all oil vectors.

Sample size: 56,000 (key, Lambda) pairs, each with all 1,024 vinegar tuples.
Vectors and matrix rows are Python integers used as bit masks.  The 20 seeds
are independent chunks, spread over up to three worker processes
(multiprocessing; one process if that is unavailable) and merged in seed
order, so the output does not depend on the number of processes.  The whole
script takes a few seconds.

Not shown: a violation of the final inequality.  In the toy no sampled pair
has a bad fraction above B, so Pr[Game5] >= Pr[Game4] (1 - B) holds for every
event on the sample.  Nothing is computed at the submitted parameter sets, no
adversary is run, and no submission code is built or run.  Python standard
library only, plus pdftotext for step 1.

Exit status: 0 PASS, 1 FAIL, 77 SKIP (pdftotext is missing).
"""

import argparse
import hashlib
import json
import math
import random
import shutil
import statistics
import subprocess
import unicodedata
from collections import Counter
from fractions import Fraction
from pathlib import Path


HERE = Path(__file__).resolve().parent
SPEC = HERE / "MAYO-spec.pdf"
SPEC_SHA256 = "1fc7561dc8ffdbe36c101b985266e4d3c8aa88fa580e18ead215a80ac7bb371f"
CANDIDATE = "MAYO-5"

# Toy parameter set (specification Section 2.1.1 with q = 2).
Q, M, OD, K, NV = 2, 4, 3, 2, 5          # field size, m, o, k, n - o
N = NV + OD
F_POLY = 0b10011                          # f(z) = z^4 + z + 1
Z = ((0b0001, 0b0010), (0b0010, 0b1000))  # [[1, z], [z, z^3]] over GF(2)[z]/(f)
TUPLES = Q ** (K * NV)                    # vinegar tuples (v_0, v_1)

# The review's sample: 20 seeds, 700 keys each, 4 Lambda per key.
SEEDS = range(1000, 1020)
KEYS_PER_SEED = 700
LAMBDAS_PER_KEY = 4
RECOUNT_FIRST = 8                         # pairs of the first seed recounted slowly

# Figures stated for this item in the review of 2026-11-15.
REVIEW = {
    "pairs": 56000,
    "bad_fraction_min": 0.148,
    "bad_fraction_max": 0.336,
    "bad_fraction_mean": 0.2165,
    "bad_fraction_sd": 0.0128,
    "B": 0.375,
}

# Text as pdftotext -layout prints it, with the physical PDF page (printed
# page = physical - 1).  Comparison ignores white space.
SPEC_QUOTES = {
    "q_is_16": (7, "In this specification, we fix q = 16."),
    "m_even": (7, "We choose it to be even."),
    "o_constraint": (7, "o, the dimension of the oil space O, satisfying n − o ≥ m."),
    "k_constraints": (
        7, "k , the whipping parameter, satisfying k < n − o, ko ≥ m, and k(k + 1)/2 ≤ m."),
    "f_irreducible": (7, "an irreducible polynomial of degree m"),
    "z_matrix": (
        7, "Z, a symmetric k × k MDS matrix over K whose k(k + 1)/2 upper diagonal "
           "entries are linearly independent over Fq ."),
    "e_matrices": (
        8, "the matrix that corresponds to multiplication by Z[i, j] mod f (z)."),
    "alg7_line_31": (15, "31: M0 ← M0 + Λv O + Λo"),
    "alg7_line_37": (
        15, "A[:, i ∗ o : (i + 1) ∗ o] ← A[:, i ∗ o : (i + 1) ∗ o] + Eij Mj"),
    "alg7_line_39": (
        15, "A[:, j ∗ o : (j + 1) ∗ o] ← A[:, j ∗ o : (j + 1) ∗ o] + Eij Mi"),
    "alg7_line_42": (15, "42: x ← SampleSolution(A, y, r)"),
    "definition_2_linear_term": (27, "L(Λ, s0 , · · · , sk−1 ) := Ei0 Λ(si ) ,"),
    "theorem_1_premise": (
        28, "be the bound on the failing probability from Lemma 1 and suppose Qs B < 1"),
    "lemma_1_fixed_linear_map": (28, "be any fixed linear map."),
    "lemma_1_average_over_o_p_v": (
        28, "are chosen uniformly at random, then as a function of {oi }i∈[k] ∈ O "
            "the affine map"),
    "lemma_1_bound": (28, "has full rank except with probability bounded by"),
    "game_3_vinegar_sampling": (
        29, "fresh signing query is made the game chooses a random Λ, and then "
            "repeatedly picks"),
    "step_6_game_5": (
        30, "6. Game5 is the same as Game4 , but the game picks s = v+o uniformly at "
            "random instead of o being"),
    "step_6_identity": (
        30, "Then, Game5 , conditioned on Good v happening, is identical to Game4 ."),
    "step_6_lemma_1_union_bound": (
        30, "does not have full rank for a single v is at most B, so, by the union "
            "bound,"),
    "step_6_inequality_good_v": (30, "≥ Pr[Game4 () = 1] Pr[Good v]"),
    "step_6_inequality_final": (30, "≥ Pr[Game4 () = 1](1 − Qs B) ."),
}


def squash(text):
    """Drop all white space; fold ligatures and the Unicode minus sign."""
    text = unicodedata.normalize("NFKC", text).replace("−", "-")
    return "".join(text.split())


# --------------------------------------------------------------------------
# 1. Specification text
# --------------------------------------------------------------------------

def spec_pages(tool):
    """Squashed text of each physical page; empty if pdftotext fails."""
    try:
        text = subprocess.run(
            [tool, "-layout", str(SPEC), "-"], capture_output=True, check=True,
            timeout=120,
        ).stdout.decode("utf-8", "replace")
    except (OSError, subprocess.SubprocessError):
        return []
    return [squash(page) for page in text.split("\f")]


def tool_version(tool):
    try:
        banner = subprocess.run(
            [tool, "-v"], capture_output=True, timeout=30,
        ).stderr.decode("utf-8", "replace")
    except (OSError, subprocess.SubprocessError):
        return "unknown"
    return banner.splitlines()[0].strip() if banner.strip() else "unknown"


def check_spec(pages):
    found = {
        name: number <= len(pages) and squash(quote) in pages[number - 1]
        for name, (number, quote) in SPEC_QUOTES.items()
    }
    step6 = ("step_6_game_5", "step_6_identity", "step_6_lemma_1_union_bound",
             "step_6_inequality_good_v", "step_6_inequality_final")
    lemma1 = ("lemma_1_fixed_linear_map", "lemma_1_average_over_o_p_v", "lemma_1_bound")
    checks = {
        "spec_step_6_identity_and_inequality_on_physical_page_30":
            all(found[name] for name in step6),
        "spec_lemma_1_is_an_average_over_o_p_v_on_physical_page_28":
            all(found[name] for name in lemma1),
        "spec_parameter_rules_and_algorithm_7_lines_used_by_the_toy": all(
            found[name] for name in found if name not in step6 + lemma1),
    }
    return checks, found


# --------------------------------------------------------------------------
# 2. Toy MAYO over GF(2)
# --------------------------------------------------------------------------
# A vector of GF(2)^M is an M-bit mask, bit a being the coefficient of z^a,
# which is also row a of the linear system (one row per public polynomial).

def field_mul(a, b):
    """Product in GF(2)[z]/(f)."""
    out = 0
    while b:
        if b & 1:
            out ^= a
        b >>= 1
        a <<= 1
        if a >> M & 1:
            a ^= F_POLY
    return out


# E[i][j][x] = E_ij x: multiplication by Z[i][j] (Section 2.1.1).
E = [[[field_mul(Z[i][j], x) for x in range(1 << M)] for j in range(K)] for i in range(K)]
E00, E01, E11 = E[0][0], E[0][1], E[1][1]


def rank(vectors):
    """Rank over GF(2) of a list of bit masks."""
    basis = []
    for vector in vectors:
        for element in basis:
            vector = min(vector, vector ^ element)
        if vector:
            basis.append(vector)
    return len(basis)


def poly_mod(a, b):
    """Remainder of the GF(2) polynomial a modulo b (bit masks)."""
    while a.bit_length() >= b.bit_length():
        a ^= b << (a.bit_length() - b.bit_length())
    return a


def toy_structure_checks():
    """The requirements of Section 2.1.1 other than q = 16, and that the matrix
    E of Lemma 1 is nonsingular."""
    irreducible = all(
        poly_mod(F_POLY, divisor) != 0
        for divisor in range(2, 1 << (M // 2 + 1))       # all of degree 1 .. m/2
    )
    upper = [Z[i][j] for i in range(K) for j in range(i, K)]
    determinant = field_mul(Z[0][0], Z[1][1]) ^ field_mul(Z[0][1], Z[1][0])
    # The 2m x 2m matrix E of Lemma 1, as 2m columns of 2m bits.
    e_columns = [E[0][j][1 << b] | E[1][j][1 << b] << M for j in range(K) for b in range(M)]
    return {
        "m_even": M % 2 == 0,
        "n_minus_o_at_least_m": NV >= M,
        "k_below_n_minus_o": K < NV,
        "ko_at_least_m": K * OD >= M,
        "k_k_plus_1_over_2_at_most_m": K * (K + 1) // 2 <= M,
        "f_irreducible_of_degree_m": F_POLY.bit_length() - 1 == M and irreducible,
        "Z_symmetric": all(Z[i][j] == Z[j][i] for i in range(K) for j in range(K)),
        "Z_mds": all(upper) and determinant != 0,       # every minor of a 2 x 2 matrix
        "Z_upper_entries_independent_over_Fq": rank(upper) == len(upper),
        "E_of_lemma_1_nonsingular": rank(e_columns) == K * M,
    }


def parity(x):
    """Parity of the number of set bits of x."""
    return bin(x).count("1") & 1


def bits(rng, positions):
    """One random bit for each position, drawn in the given order."""
    return sum(rng.getrandbits(1) << j for j in positions)


def keygen(rng):
    """Uniform O, P1 (upper triangular) and P2, as lists of row masks.

    The order of the random draws is that of the review's script, so that the
    same seeds give the same keys.
    """
    oil = [bits(rng, range(OD)) for _ in range(NV)]                # (n-o) x o
    p1s, p2s = [], []
    for _ in range(M):
        p1s.append([bits(rng, range(i, NV)) for i in range(NV)])   # (n-o) x (n-o)
        p2s.append([bits(rng, range(OD)) for _ in range(NV)])      # (n-o) x o
    return oil, p1s, p2s


def times_oil(row, oil):
    """row * O for a row over the vinegar coordinates."""
    out = 0
    for t in range(NV):
        if row >> t & 1:
            out ^= oil[t]
    return out


def l_matrices(oil, p1s, p2s):
    """L_a = (P1_a + P1_a^T) O + P2_a (Algorithm 5, line 17), as row masks."""
    out = []
    for p1, p2 in zip(p1s, p2s):
        transposed = [sum((p1[t] >> i & 1) << t for t in range(NV)) for i in range(NV)]
        out.append([times_oil(p1[i] ^ transposed[i], oil) ^ p2[i] for i in range(NV)])
    return out


def draw_lambda(rng):
    """Lambda in GF(2)^(m x n) as m row masks: bits 0..n-o-1 are Lambda_v, the
    rest Lambda_o."""
    return [bits(rng, range(N)) for _ in range(M)]


def columns_of(rows):
    """The o columns (m-bit masks) of an m x o matrix given as row masks."""
    return tuple(sum((rows[a] >> j & 1) << a for a in range(M)) for j in range(OD))


def vinegar_table(ls):
    """For each vinegar vector v, the o columns of M(v), M(v)[a, :] = v^T L_a
    (Algorithm 7, line 30)."""
    unit = [columns_of([ls[a][i] for a in range(M)]) for i in range(NV)]
    table = [(0,) * OD]
    for v in range(1, Q ** NV):
        low = (v & -v).bit_length() - 1
        rest = table[v & (v - 1)]
        table.append(tuple(rest[j] ^ unit[low][j] for j in range(OD)))
    return table


def lambda_columns(oil, lam):
    """The o columns of Lambda_v O + Lambda_o (Algorithm 7, line 31)."""
    return columns_of([times_oil(row, oil) ^ row >> NV for row in lam])


def good_tuples_direct(table, lam_cols):
    """Number of (v_0, v_1) for which A of Algorithm 7 has rank m, by building A
    (lines 25-39) and reducing it."""
    good = 0
    for v0 in range(Q ** NV):
        m0 = [table[v0][j] ^ lam_cols[j] for j in range(OD)]            # line 31
        for v1 in range(Q ** NV):
            m1 = table[v1]
            block0 = [E00[m0[j]] ^ E01[m1[j]] for j in range(OD)]       # lines 37, 39
            block1 = [E01[m0[j]] ^ E11[m1[j]] for j in range(OD)]
            good += rank(block0 + block1) == M
    return good


# Fast enumeration.  Column j of A contributes the two vectors
# E00 a + E01 b and E01 a + E11 b, with a = M_0[:, j] and b = M_1[:, j].  A has
# rank m when the spans of the o pairs add up to GF(2)^m.  The 67 subspaces of
# GF(2)^4 are numbered, and joins are looked up.

def subspace_tables():
    def add(space, vector):                 # span of space and vector, as a 2^m-bit set
        return space | sum(1 << (e ^ vector) for e in range(1 << M) if space >> e & 1)

    spaces, frontier = {1}, [1]             # {0}
    while frontier:
        grown = {add(s, x) for s in frontier for x in range(1, 1 << M)} - spaces
        spaces |= grown
        frontier = list(grown)
    spaces = sorted(spaces)
    index = {space: i for i, space in enumerate(spaces)}

    def join(a, b):
        for e in range(1 << M):
            if b >> e & 1:
                a = add(a, e)
        return a

    joins = [[index[join(a, b)] for b in spaces] for a in spaces]
    pair = [
        [index[add(add(1, E00[a] ^ E01[b]), E01[a] ^ E11[b])] for b in range(1 << M)]
        for a in range(1 << M)
    ]
    return len(spaces), joins, pair, index[(1 << (1 << M)) - 1]


SUBSPACES, JOIN, PAIR, FULL = subspace_tables()


def good_tuples(table, lam_cols):
    """Number of (v_0, v_1) for which A of Algorithm 7 has rank m."""
    l0, l1, l2 = lam_cols
    join, full, good = JOIN, FULL, 0
    for a0, a1, a2 in table:
        r0, r1, r2 = PAIR[a0 ^ l0], PAIR[a1 ^ l1], PAIR[a2 ^ l2]
        good += sum(
            1 for b0, b1, b2 in table if join[join[r0[b0]][r1[b1]]][r2[b2]] == full
        )
    return good


def good_tuples_from_definition(oil, p1s, p2s, lam):
    """Number of (v_0, v_1) for which o -> P*(v + o) + L(Lambda, v + o) of
    Definition 2 is onto GF(2)^m, by evaluating it at every o in O^k.

    Uses P_a = [[P1_a, P2_a], [0, P3_a]] with P3_a = Upper(O^T P1_a O + O^T P2_a)
    (Algorithm 4, line 16; signs vanish in characteristic 2), not L_a.
    Also returns whether P vanishes on the oil space.
    """
    forms = []                              # per polynomial, n upper-triangular row masks
    for p1, p2 in zip(p1s, p2s):
        inner = [times_oil(p1[t], oil) ^ p2[t] for t in range(NV)]       # P1 O + P2
        square = [0] * OD                                                # O^T (P1 O + P2)
        for t in range(NV):
            for i in range(OD):
                if oil[t] >> i & 1:
                    square[i] ^= inner[t]
        p3 = [
            (square[i] >> i & 1) << i
            | sum(((square[i] >> j ^ square[j] >> i) & 1) << j for j in range(i + 1, OD))
            for i in range(OD)                                           # Upper()
        ]
        forms.append([p1[i] | p2[i] << NV for i in range(NV)] + [row << NV for row in p3])

    def evaluate(x):                        # P(x) as an m-bit mask
        return sum(
            (sum(parity(form[i] & x) for i in range(N) if x >> i & 1) & 1) << a
            for a, form in enumerate(forms)
        )

    p_table = [evaluate(x) for x in range(1 << N)]
    lam_table = [sum(parity(lam[a] & x) << a for a in range(M)) for x in range(1 << N)]
    # The oil space: (O u, u) for u in GF(2)^o; vinegar coordinates are bits 0..n-o-1.
    oil_vectors = [
        sum(parity(oil[t] & u) << t for t in range(NV)) | u << NV for u in range(1 << OD)
    ]
    vanishes = all(p_table[x] == 0 for x in oil_vectors)

    good = 0
    for v0 in range(Q ** NV):
        for v1 in range(Q ** NV):
            image = set()
            for o0 in oil_vectors:
                s0 = v0 ^ o0
                first = E00[p_table[s0] ^ lam_table[s0]]       # E_00 P(s_0) + E_00 Lambda s_0
                for o1 in oil_vectors:
                    s1 = v1 ^ o1
                    polar = p_table[s0 ^ s1] ^ p_table[s0] ^ p_table[s1]      # P'(s_0, s_1)
                    image.add(first ^ E11[p_table[s1]] ^ E01[polar ^ lam_table[s1]])
            good += len(image) == 1 << M
    return good, vanishes


# --------------------------------------------------------------------------
# 3. The average-case quantity of Lemma 1, exactly
# --------------------------------------------------------------------------
# For a uniform key the L_a are uniform, so M(v_1) is uniform when v_1 != 0,
# and M(v_0) + Lambda_v O + Lambda_o is uniform and independent of it because
# Lambda is.  E is nonsingular, so A is then a uniform m x ko matrix.  When
# v_1 = 0 the matrix is [E00 X | E01 X] with X uniform.

def exact_average_bad_fraction():
    full_rank = math.prod(Fraction(Q ** (K * OD) - Q ** i, Q ** (K * OD)) for i in range(M))
    p_uniform = 1 - full_rank
    singular = sum(
        rank([E00[x] for x in columns] + [E01[x] for x in columns]) < M
        for columns in ((a, b, c) for a in range(1 << M) for b in range(1 << M)
                        for c in range(1 << M))
    )
    p_v1_zero = Fraction(singular, (1 << M) ** OD)
    weight = Fraction(1, Q ** NV)
    return (1 - weight) * p_uniform + weight * p_v1_zero, p_uniform, p_v1_zero


LAMBDA_ROWS = [Counter(PAIR[a][b] for a in range(1 << M)) for b in range(1 << M)]
LAMBDA_SUM_MEMO = {}


def lambda_sum(columns):
    """g(b): the number of a in GF(16)^o for which the columns (a_j, b_j) give
    rank m.  For one key, the sum over all Lambda_v O + Lambda_o of the number
    of good tuples is q^(n-o) times the sum over v_1 of g(M(v_1))."""
    if columns not in LAMBDA_SUM_MEMO:
        c0, c1, c2 = (LAMBDA_ROWS[b] for b in columns)
        LAMBDA_SUM_MEMO[columns] = sum(
            n0 * n1 * n2
            for s0, n0 in c0.items() for s1, n1 in c1.items() for s2, n2 in c2.items()
            if JOIN[JOIN[s0][s1]][s2] == FULL
        )
    return LAMBDA_SUM_MEMO[columns]


# --------------------------------------------------------------------------
# The sample: one chunk per seed
# --------------------------------------------------------------------------

def sample_chunk(seed):
    """All keys and Lambda of one seed.

    Returns the good-tuple count of every pair, the per-key sums over all
    Lambda (divided by q^(n-o)), the first pairs, and the first pair with the
    most and with the fewest good tuples, each with its key and Lambda.
    """
    rng = random.Random(seed)
    good, key_sums, first, most, fewest = [], [], [], None, None
    for key_number in range(KEYS_PER_SEED):
        oil, p1s, p2s = keygen(rng)
        table = vinegar_table(l_matrices(oil, p1s, p2s))
        key_sums.append(sum(lambda_sum(columns) for columns in table))
        for lambda_number in range(LAMBDAS_PER_KEY):
            lam = draw_lambda(rng)
            count = good_tuples(table, lambda_columns(oil, lam))
            pair = (count, f"seed {seed}, key {key_number}, Lambda {lambda_number}",
                    oil, p1s, p2s, lam)
            if len(good) < RECOUNT_FIRST:
                first.append(pair)
            if most is None or count > most[0]:
                most = pair
            if fewest is None or count < fewest[0]:
                fewest = pair
            good.append(count)
    return good, key_sums, first, most, fewest


def run_sample(jobs):
    """The chunks in seed order, and the number of processes used."""
    seeds = list(SEEDS)
    if jobs > 1:
        try:
            import multiprocessing
            with multiprocessing.Pool(jobs) as pool:
                return pool.map(sample_chunk, seeds, chunksize=1), jobs
        except Exception:                   # no multiprocessing here: one process
            pass
    return [sample_chunk(seed) for seed in seeds], 1


# --------------------------------------------------------------------------

def frac(value):
    return f"{value.numerator}/{value.denominator}"


def emit(result, output):
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if output:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(encoded)
    print(encoded, end="")
    return {"PASS": 0, "SKIP": 77}.get(result["status"], 1)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", type=Path)
    parser.add_argument("--jobs", type=int, default=3, choices=(1, 2, 3),
                        help="worker processes for the sample (default 3)")
    args = parser.parse_args()

    result = {
        "candidate": CANDIDATE,
        "evidence": (
            "Exact enumeration of all vinegar tuples on a toy MAYO (GF(2), m=4, o=3, "
            "k=2, n-o=5; the review's 14,000 keys times 4 Lambda) shows that the "
            "good-vinegar fraction gamma(P,O,Lambda) is not constant (bad fraction "
            "0.148 to 0.336) while its sample average 0.21653 agrees, within sampling "
            "error, with the exactly computed average-case value 0.21655 that Lemma 1 "
            "bounds by B = 0.375, so 'Game5, conditioned on Good v happening, is "
            "identical to Game4' (specification physical p.30, step 6) does not hold "
            "as written for the toy; no violation of the final inequality "
            "Pr[Game5] >= Pr[Game4](1 - Qs B) is shown (no sampled pair exceeds B), "
            "nothing is computed at the submitted parameter sets, and no submission "
            "code is built or run."
        ),
        "source_sha256": {"MAYO/MAYO-spec.pdf": SPEC_SHA256},
        "checks": {},
        "measurements": {},
    }
    tool = shutil.which("pdftotext")
    if tool is None:
        result["status"] = "SKIP"
        result["reason"] = "pdftotext not found"
        return emit(result, args.output)

    checks = {
        "pinned_specification":
            SPEC.is_file() and hashlib.sha256(SPEC.read_bytes()).hexdigest() == SPEC_SHA256,
    }

    # 1. Specification text.
    pages = spec_pages(tool) if checks["pinned_specification"] else []
    spec_checks, found = check_spec(pages)
    checks.update(spec_checks)

    # 2. Toy enumeration on the review's sample.
    structure = toy_structure_checks()
    checks["toy_meets_section_2_1_1_except_q"] = all(structure.values())
    bound_b = Fraction(Q ** K, Q ** NV * (Q - 1)) + Fraction(Q ** M, Q ** (K * OD) * (Q - 1))

    chunks, processes = run_sample(args.jobs)
    good = [count for chunk in chunks for count in chunk[0]]       # per (key, Lambda)
    key_sums = [total for chunk in chunks for total in chunk[1]]   # per key, all Lambda
    most = max((chunk[3] for chunk in chunks), key=lambda pair: pair[0])     # first of ties
    fewest = min((chunk[4] for chunk in chunks), key=lambda pair: pair[0])
    recount = [(f"sample index {i}", pair) for i, pair in enumerate(chunks[0][2])]
    recount += [("fewest bad tuples", most), ("most bad tuples", fewest)]

    pairs, keys = len(good), len(key_sums)
    bad = [1 - count / TUPLES for count in good]
    bad_mean = Fraction(sum(TUPLES - count for count in good), TUPLES * pairs)
    bad_sd = statistics.pstdev(bad)
    by_key = [good[i:i + LAMBDAS_PER_KEY] for i in range(0, pairs, LAMBDAS_PER_KEY)]
    keys_with_varying_gamma = sum(len(set(group)) > 1 for group in by_key)

    # Recounts by the two slower methods.
    direct_agree, definition_agree, vanishes_on_oil = True, True, True
    recounts = []
    for which, (count, label, oil, p1s, p2s, lam) in recount:
        table = vinegar_table(l_matrices(oil, p1s, p2s))
        direct = good_tuples_direct(table, lambda_columns(oil, lam))
        by_definition, vanishes = good_tuples_from_definition(oil, p1s, p2s, lam)
        direct_agree &= direct == count
        definition_agree &= by_definition == count
        vanishes_on_oil &= vanishes
        recounts.append({
            "pair": label,
            "which": which,
            "good_tuples": count,
            "good_tuples_direct_rank_of_A": direct,
            "good_tuples_from_definition_2": by_definition,
            "bad_fraction": frac(Fraction(TUPLES - count, TUPLES)),
        })

    # One key, every value of Lambda_v O + Lambda_o: gamma varies with Lambda.
    first_key = vinegar_table(l_matrices(*keygen(random.Random(SEEDS[0]))))
    one_key = [
        good_tuples(first_key, (a, b, c))
        for a in range(1 << M) for b in range(1 << M) for c in range(1 << M)
    ]
    one_key_sum_matches = sum(one_key) == Q ** NV * key_sums[0]

    # Per key, the exact average over all Lambda: gamma varies between keys.
    lambdas = (1 << M) ** OD
    key_bad = [1 - Fraction(total, Q ** NV * lambdas) for total in key_sums]
    key_bad_mean = sum(key_bad) / keys

    # 3. Control: the average-case quantity of Lemma 1.
    exact_bad, p_uniform, p_v1_zero = exact_average_bad_fraction()
    every_b = [(a, b, c) for a in range(1 << M) for b in range(1 << M) for c in range(1 << M)]
    exact_good_by_table = Fraction(
        lambda_sum((0, 0, 0))
        + (Q ** NV - 1) * Fraction(sum(lambda_sum(b) for b in every_b), lambdas),
        Q ** NV * lambdas,
    )
    pair_means = [1 - sum(group) / (LAMBDAS_PER_KEY * TUPLES) for group in by_key]
    pair_se = statistics.pstdev(pair_means) / math.sqrt(keys)       # keys are independent
    key_se = statistics.pstdev([float(x) for x in key_bad]) / math.sqrt(keys)
    z_pairs = float(bad_mean - exact_bad) / pair_se
    z_keys = float(key_bad_mean - exact_bad) / key_se

    # Game 4 against Game 5 given Good v, one signing query, on the sample:
    # Game 4 gives every sampled (key, Lambda) weight 1/n, Game 5 given Good v
    # gives it weight gamma / sum(gamma).  E is "at least a quarter of the
    # vinegar tuples are bad".
    total_good = sum(good)
    in_event = [count for count in good if 4 * (TUPLES - count) >= TUPLES]
    pr_game4 = Fraction(len(in_event), pairs)
    pr_game5_given_good = Fraction(sum(in_event), total_good)
    pr_game5_and_good = Fraction(sum(in_event), pairs * TUPLES)
    total_variation = sum(abs(Fraction(count, total_good) - Fraction(1, pairs)) for count in good) / 2
    above_b = sum(Fraction(TUPLES - count, TUPLES) > bound_b for count in good)

    checks.update({
        "table_enumeration_equals_direct_rank_of_A_on_recounted_pairs": direct_agree,
        "table_enumeration_equals_definition_2_on_recounted_pairs":
            definition_agree and vanishes_on_oil,
        "gamma_is_not_constant_on_the_sample": len(set(good)) > 1,
        "gamma_varies_with_lambda_for_a_fixed_key":
            len(set(one_key)) > 1 and one_key_sum_matches and keys_with_varying_gamma > 0,
        "lambda_averaged_gamma_varies_between_keys": len(set(key_sums)) > 1,
        "game5_given_good_v_weights_keys_differently_from_game4":
            pr_game5_given_good < pr_game4 and total_variation > 0,
        # Control: on average the toy behaves as Lemma 1 says.
        "control_exact_average_bad_fraction_is_at_most_B":
            exact_bad <= bound_b and 1 - exact_good_by_table == exact_bad,
        "control_sample_average_matches_exact_average_case_value":
            abs(z_pairs) < 4 and abs(z_keys) < 4,
        # Stated limit of the review: the final inequality is not violated.
        "limit_no_sampled_pair_has_bad_fraction_above_B":
            above_b == 0 and pr_game5_and_good >= pr_game4 * (1 - bound_b),
        "figures_match_review": (
            pairs == REVIEW["pairs"]
            and round(min(bad), 3) == REVIEW["bad_fraction_min"]
            and round(max(bad), 3) == REVIEW["bad_fraction_max"]
            and round(float(bad_mean), 4) == REVIEW["bad_fraction_mean"]
            and round(bad_sd, 4) == REVIEW["bad_fraction_sd"]
            and float(bound_b) == REVIEW["B"]
        ),
    })

    result.update({
        "checks": checks,
        "controls": sorted(name for name in checks if name.startswith("control_")),
        "measurements": {
            "toy": {
                "q": Q, "m": M, "o": OD, "k": K, "n_minus_o": NV, "n": N,
                "f": "z^4 + z + 1", "Z": "[[1, z], [z, z^3]]",
                "vinegar_tuples_per_pair": TUPLES,
                "subspaces_of_GF(2)^4": SUBSPACES,
                "structure": structure,
                "B_of_lemma_1": frac(bound_b),
            },
            "sample": {
                "seeds": [SEEDS[0], SEEDS[-1]],
                "keys_per_seed": KEYS_PER_SEED,
                "keys": keys,
                "lambda_per_key": LAMBDAS_PER_KEY,
                "pairs": pairs,
                "vinegar_tuples_enumerated": pairs * TUPLES,
                "worker_processes": processes,
            },
            "bad_fraction_over_pairs": {
                "min": round(min(bad), 6),
                "max": round(max(bad), 6),
                "mean": round(float(bad_mean), 6),
                "standard_deviation": round(bad_sd, 6),
                "distinct_values": len(set(good)),
                "pairs_above_B": above_b,
            },
            "varies_with_lambda": {
                "sampled_keys_whose_4_lambda_give_more_than_one_gamma": keys_with_varying_gamma,
                "first_key_over_all_4096_values_of_lambda_on_the_oil_space": {
                    "bad_fraction_min": round(1 - max(one_key) / TUPLES, 6),
                    "bad_fraction_max": round(1 - min(one_key) / TUPLES, 6),
                    "distinct_values": len(set(one_key)),
                },
            },
            "varies_between_keys_exact_average_over_all_lambda": {
                "bad_fraction_min": round(float(min(key_bad)), 6),
                "bad_fraction_max": round(float(max(key_bad)), 6),
                "mean": round(float(key_bad_mean), 6),
                "distinct_values": len(set(key_sums)),
            },
            "recounts": recounts,
            "control_average_case": {
                "exact_average_bad_fraction": frac(exact_bad),
                "exact_average_bad_fraction_decimal": round(float(exact_bad), 6),
                "uniform_m_by_ko_matrix_rank_deficient": frac(p_uniform),
                "rank_deficient_when_v1_is_zero": frac(p_v1_zero),
                "B_of_lemma_1": round(float(bound_b), 6),
                "sample_mean_minus_exact": float(bad_mean - exact_bad),
                "sample_mean_minus_exact_in_standard_errors": round(z_pairs, 2),
                "per_key_lambda_average_minus_exact": float(key_bad_mean - exact_bad),
                "per_key_lambda_average_minus_exact_in_standard_errors": round(z_keys, 2),
            },
            "game4_against_game5_given_good_v_one_query": {
                "event": "at least 1/4 of the vinegar tuples are bad",
                "pr_game4": round(float(pr_game4), 6),
                "pr_game5_given_good_v": round(float(pr_game5_given_good), 6),
                "pr_game5_and_good_v": round(float(pr_game5_and_good), 6),
                "pr_game4_times_1_minus_B": round(float(pr_game4 * (1 - bound_b)), 6),
                "total_variation_of_the_two_weightings_of_the_sample":
                    round(float(total_variation), 6),
            },
        },
        "review_figures": REVIEW,
        "spec": {
            "extracted_with": tool_version(tool) + ", option -layout",
            "pages_are": "physical PDF pages (printed page = physical - 1)",
            "quotes": {
                name: {"page": page, "text": quote, "found": found[name]}
                for name, (page, quote) in SPEC_QUOTES.items()
            },
        },
        "not_covered": [
            "No violation of Pr[Game5] >= Pr[Game4](1 - Qs B) is shown: no sampled "
            "pair has a bad fraction above B, so for one signing query the "
            "inequality holds for every event on the sample.",
            "Nothing is computed at the submitted parameter sets (q = 16, m = 64 to "
            "142); how tightly gamma concentrates there is not measured.",
            "The toy uses q = 2; the specification fixes q = 16.",
            "No adversary is run; the Game 4 and Game 5 figures are weights of the "
            "sampled (key, Lambda) pairs for one signing query.",
            "Lemma 1 is not re-derived; its average is computed exactly for the toy.",
            "No repair of the step is proved here.",
            "No submission code is built, run or pinned.",
        ],
        "status": "PASS" if all(checks.values()) else "FAIL",
    })
    return emit(result, args.output)


if __name__ == "__main__":
    raise SystemExit(main())
