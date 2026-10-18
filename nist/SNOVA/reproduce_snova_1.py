#!/usr/bin/env python3
"""SNOVA-1: exact-source sampler witness, not a full-size key-recovery run.

Compile the unmodified submitted expand_gf body against its submitted header.
Check exact digit counts algebraically, boundary C outputs, and controls.
No network access or submitted build script is needed.
"""
import argparse
import ctypes
import hashlib
import json
import math
from pathlib import Path
import random
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_S/snova_ref.c'
HEADER = SOURCE.with_name('snova.h')
PINS = {
    'snova_ref.c': 'ef5dc33896f799c19631f888dd8c9192565e219df54e7932849188ae2e7e117c',
    'snova.h': '91b38ca19ec6f12d34fb87a1cf5e299c5e71d4c96199f47e8714472579e5274c',
}
PACK = {11: (16, 7), 13: (15, 7), 16: (2, 1), 19: (15, 8)}


def extract_function(text):
    start = text.index('static int expand_gf(')
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def digit_counts(q, batch, num_bytes, digit):
    """Count each base-q digit among every possible uniform byte block."""
    space = 256 ** num_bytes
    width = q ** digit
    cycles, remainder = divmod(space, q * width)
    return [cycles * width + min(width, max(0, remainder - value * width))
            for value in range(q)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    compiler = shutil.which('cc')
    if not compiler:
        print(json.dumps({'status': 'SKIP', 'reason': 'C compiler unavailable'}))
        return 77
    for path in [SOURCE, HEADER]:
        assert hashlib.sha256(path.read_bytes()).hexdigest() == PINS[path.name], path
    text = SOURCE.read_text()
    # Check that the production signer feeds the biased byte expansion into V.
    assert 'expand_gf(signature_in_GF, vinegar_in_byte, SNOVA_v * SNOVA_lr);' in text
    assert 'shake_squeeze(vinegar_in_byte, NUM_GEN_SEC_BYTES, &v_instance);' in text
    assert 'REJECTION_LIMIT' in text and 'T12coef[t_idx] = sk_data[idx] % SNOVA_q;' in text
    body = extract_function(text)
    result = {'id': 'SNOVA-1', 'status': 'PASS',
              'evidence': 'exact_source_sampler_and_distribution_controls',
              'full_parameter_key_recovery': False,
              'source_sha256': PINS, 'fields': []}
    recommended = []
    for params in sorted(SOURCE.parents[1].glob('SNOVA_*/snova_params.h')):
        q = int(__import__('re').search(r'#define SNOVA_q (\d+)', params.read_text()).group(1))
        assert q == 16, params
        recommended.append(params.parent.name)
    assert len(recommended) == 18
    result['unaffected_recommended_reference_instances'] = recommended
    rng = random.Random(0x534e4f5641)
    with tempfile.TemporaryDirectory(prefix='snova-1-') as temporary:
        temporary = Path(temporary)
        for q, (batch, num_bytes) in PACK.items():
            c_source = temporary / f'sampler-{q}.c'
            library = temporary / f'sampler-{q}.so'
            c_source.write_text('''/* SNOVA TEAM MIT-licensed expand_gf body,
 * extracted without changes for this independent witness. */
#include "snova.h"
typedef uint8_t gf_t;
''' + body + '\nint witness(uint8_t *out, const uint8_t *in, size_t n) {return expand_gf(out,in,n);}\n')
            subprocess.run([compiler, '-std=c11', '-O2', '-shared', '-fPIC',
                            '-I' + str(HEADER.parent), '-DSNOVA_q=' + str(q),
                            '-DSNOVA_v=27', '-DSNOVA_o=5', '-DSNOVA_l=4',
                            '-DSNOVA_r=6', '-DSNOVA_m1=5', str(c_source),
                            '-o', str(library)], check=True, capture_output=True)
            loaded = ctypes.CDLL(str(library))
            loaded.witness.argtypes = [ctypes.POINTER(ctypes.c_uint8),
                                      ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t]
            loaded.witness.restype = ctypes.c_int
            space = 256 ** num_bytes
            modulus = q ** batch
            candidates = {0, 1, space - 1, modulus - 1}
            if modulus < space:
                candidates.update([modulus, modulus + 1])
            for digit in range(batch):
                for value in range(q):
                    boundary = value * q ** digit
                    for candidate in [boundary - 1, boundary, boundary + 1]:
                        if 0 <= candidate < space:
                            candidates.add(candidate)
            candidates.update(rng.randrange(space) for _ in range(100))
            for candidate in sorted(candidates):
                raw = candidate.to_bytes(num_bytes, 'little')
                encoded = (ctypes.c_uint8 * num_bytes).from_buffer_copy(raw)
                decoded = (ctypes.c_uint8 * batch)()
                remainder = loaded.witness(decoded, encoded, batch)
                assert list(decoded) == [(candidate // q ** digit) % q
                                         for digit in range(batch)]
                assert bool(remainder) == bool(candidate // modulus)
            if q == 13:
                # Deliberately tiny LPN model using noise from the exact C
                # sampler. This is not the full SNOVA signature distribution.
                toy_rng = random.Random(0x42574153)
                true_key = (3, 9)
                samples = 3000
                heavy_set = set(range(5))
                block_limit = (space // modulus) * modulus
                datasets = {"submitted": [], "uniform_rejection": []}
                for kind in datasets:
                    for _ in range(samples):
                        value = toy_rng.randrange(space)
                        while kind == "uniform_rejection" and value >= block_limit:
                            value = toy_rng.randrange(space)
                        raw = (ctypes.c_uint8 * num_bytes).from_buffer_copy(
                            value.to_bytes(num_bytes, 'little'))
                        output = (ctypes.c_uint8 * batch)()
                        loaded.witness(output, raw, batch)
                        noise = output[batch - 1]
                        x = (toy_rng.randrange(q), toy_rng.randrange(q))
                        y = (noise - sum(a*b for a,b in zip(x,true_key))) % q
                        datasets[kind].append((x[0],x[1],y))
                scores = {}
                for kind, dataset in datasets.items():
                    ranking = sorted(((sum((y+a*x0+b*x1) % q in heavy_set
                                           for x0,x1,y in dataset), (a,b))
                                      for a in range(q) for b in range(q)),
                                     reverse=True)
                    true_score = next(score for score,key in ranking if key == true_key)
                    scores[kind] = {"best_key": ranking[0][1],
                                    "best_score": ranking[0][0]/samples,
                                    "second_score": ranking[1][0]/samples,
                                    "true_key_score": true_score/samples}
                assert scores['submitted']['best_key'] == true_key
                assert scores['submitted']['best_score'] > 0.49
                assert scores['uniform_rejection']['best_score'] < 0.49
                result['toy_lpn_model'] = {"q":q, "dimension":2,
                    "samples":samples, "true_key":true_key,
                    "noise_source":"exact submitted C top digit",
                    "model_only_not_full_SNOVA":True, "controls":scores}
            all_counts = [digit_counts(q, batch, num_bytes, digit)
                          for digit in range(batch)]
            assert all(sum(counts) == space for counts in all_counts)
            # Tiny exhaustive control validates the count formula independently.
            tiny = [0] * q
            for candidate in range(256):
                tiny[candidate % q] += 1
            assert tiny == digit_counts(q, 1, 1, 0)
            counts = all_counts[-1]
            divergence = sum((count / space) * math.log2(q * count / space)
                             for count in counts if count)
            accepted = (space // modulus) * modulus
            # Rejection at this boundary leaves equal preimages for all q^batch
            # tuples, hence an exactly uniform digit (no statistical tolerance).
            rejection_digit_counts = [(space // modulus) * q ** (batch - 1)] * q
            assert sum(rejection_digit_counts) == accepted
            byte_limit = (256 // q) * q
            byte_rejection = [sum(x % q == digit for x in range(byte_limit))
                              for digit in range(q)]
            assert len(set(byte_rejection)) == 1
            assert (len(set(counts)) == 1) == (q == 16)
            if q != 16:
                assert divergence > 0
            heavy = (space - modulus) // q ** (batch - 1) if q != 16 else 0
            probability = sum(counts[:heavy]) / space
            result['fields'].append({
                'q': q, 'field_elements_per_block': batch, 'bytes_per_block': num_bytes,
                'C_boundary_and_random_cases': len(candidates),
                'most_significant_digit_exact_preimage_counts': counts,
                'source_space': space, 'top_digit_KL_bits': divergence,
                'heavy_set': list(range(heavy)), 'heavy_probability': probability,
                'uniform_heavy_probability': heavy / q,
                'block_rejection_acceptance': accepted / space,
                'byte_rejection_uniform_counts': byte_rejection,
                'q16_uniform_control': q == 16,
            })
    rendered = json.dumps(result, indent=2) + '\n'
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered)
    print(rendered, end='')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
