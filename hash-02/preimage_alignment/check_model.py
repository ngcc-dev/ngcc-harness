"""E0: cross-check the Python model (axis.py) against our own compile of the reference core."""
import ctypes, os, random, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import axis as A
lib = ctypes.CDLL(os.environ['AXIS_HARNESS_LIB'])
VAR = {512: 0, 768: 1, 1024: 2}

def regs_to_c(S):
    arr = (ctypes.c_uint64 * 24)()
    for j in range(8):
        for w in range(3):
            arr[3*j+w] = (S[j] >> (64*w)) & ((1 << 64) - 1)
    return arr
def c_to_regs(arr):
    return [arr[3*j] | (arr[3*j+1] << 64) | (arr[3*j+2] << 128) for j in range(8)]

random.seed(1)
# 1. single beats (message beat and constant beat) on random states, plus inverse
for t in range(300):
    S = A.rand_state(); m0, m1 = random.getrandbits(1), random.getrandbits(1)
    arr = regs_to_c(S); lib.w_step(arr, 2, m0, m1)
    T = list(S); A.beat(T, [m0, m1]*4)
    assert c_to_regs(arr) == T
    A.beat_inv(T, [m0, m1]*4); assert T == S
    cb = random.getrandbits(8); arr = regs_to_c(S); lib.w_reupfull(arr, cb)
    T = list(S); A.beat(T, [(cb >> j) & 1 for j in range(8)]); assert c_to_regs(arr) == T
print("beat / reupfull / inverse: 300 random states OK")
# 2. constant stream
for t in range(200):
    pb, bt = random.randrange(64, 10**6, 64), random.randrange(0, 2048)
    assert lib.w_cbyte(ctypes.c_uint64(pb), ctypes.c_uint64(bt)) == sum(b << j for j, b in enumerate(A.const_ins(pb, bt)))
print("constant stream OK")
# 3. full hashes, fast (byte-aligned) and bitwise paths
lib.w_hash.argtypes = lib.w_hash_slow.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_uint64, ctypes.c_char_p]
n = 0
for v in (512, 768, 1024):
    for L in [0, 1, 7, 8, 61, 62, 63, 64, 126, 127, 128, 190, 192, 255, 256, 300, 513, 1000, 1536, 2047]:
        msg = os.urandom((L + 7)//8 + 1)
        out = ctypes.create_string_buffer(v//8); out2 = ctypes.create_string_buffer(v//8)
        lib.w_hash(VAR[v], msg, L, out); lib.w_hash_slow(VAR[v], msg, L, out2)
        mine = A.axis_hash_bits(v, A.bytes_to_bits(msg, L))
        assert out.raw == mine, (v, L)
        assert out2.raw == mine, (v, L, 'slow')
        n += 1
print(f"full digests: {n} (variant,length) cases match, fast and bitwise reference paths")
