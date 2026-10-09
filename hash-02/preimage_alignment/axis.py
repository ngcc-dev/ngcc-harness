"""Independent Python model of AXIS (hash-02), written from pseudocode.md / spec Alg. 1.

State: list of 8 Python ints, each a 192-bit register; bit i = spec bit S^j[i] (bit 0 = LSB).
Cross-checked against our own compile of the reference core in check_model.py.
"""
import os

W = 192
MASK = (1 << W) - 1
EXL = [0, 12, 32, 96, 107, 125, 11]
UPL = [66, 75, 90, 162, 178, 188]
U1 = (1 << 66) | (1 << 75) | (1 << 90)      # receives t1
U0 = (1 << 162) | (1 << 178) | (1 << 188)   # receives t0
U = U0 | U1

INIT_WORDS = [
    (0x9E3779B97F4A7C15, 0xB7E151628AED2A6A, 0x243F6A8885A308D3),
    (0x49CA76E19F938DA4, 0xD81F8650363C93F8, 0x7160C6B758B90A76),
    (0xCD71C257B752EEB5, 0xB54CBDE35B659CC7, 0x70FF7724ACB762F7),
    (0x47EA14CFA7D43857, 0x3120355B0549D6B9, 0x4FD6E6B4FBAB7ECD),
    (0x5DCD2735332E4029, 0x03F92D7B7FAEEF10, 0x6B634EF404919AAA),
    (0xD57AC41E845348D4, 0xCD0814056FA6E00B, 0x53C1A2C96C4DE7E3),
    (0xABDBF53A09476735, 0x352FF6633C6EFBA6, 0xF3FF90522F99DAE5),
    (0x0D875B906F276D2C, 0xD94570230179F812, 0xB1A270D3653FB5B8),
]
INIT = [w0 | (w1 << 64) | (w2 << 128) for (w0, w1, w2) in INIT_WORDS]


def bit(x, i):
    return (x >> i) & 1


def rotr1(x):
    """new[i] = old[i+1], new[191] = old[0]"""
    return (x >> 1) | ((x & 1) << (W - 1))


def rotl1(x):
    return ((x << 1) & MASK) | (x >> (W - 1))


def self_sbox(r):
    return (bit(r, 125) & bit(r, 32)) ^ (bit(r, 107) & bit(r, 12)) ^ (bit(r, 96) & bit(r, 0))


def tau(S, j):
    a, b, c = S[(j + 5) % 8], S[(j + 3) % 8], S[(j + 1) % 8]
    return (bit(a, 96) & bit(a, 32)) ^ (bit(b, 125) & bit(b, 12)) ^ (bit(c, 107) & bit(c, 0))


def upres(S, j, m):
    """In-place UpRes on register j with input bit m (reads current S, incl. already-updated regs)."""
    r = S[j]
    b = m ^ self_sbox(r) ^ tau(S, j) ^ bit(S[(j - 1) % 8], 11)
    t0 = bit(r, 0) ^ bit(r, 12) ^ bit(r, 32) ^ b
    t1 = bit(r, 96) ^ bit(r, 107) ^ bit(r, 125) ^ b
    if t0:
        r ^= U0
    if t1:
        r ^= U1
    S[j] = rotr1(r)


def beat(S, ins):
    """ins: list of 8 input bits (register j gets ins[j])."""
    for j in range(8):
        upres(S, j, ins[j])


def upres_inv(S, j, m):
    """Inverse of upres for register j, given that S[k] for k != j hold the values seen
    by the forward update (registers < j already updated, > j not yet)."""
    r = rotl1(S[j])            # r = old ^ t-taps; taps don't touch read positions
    # read positions {0,12,32,96,107,125} are untouched by taps -> recover b, t0, t1 from r
    b = m ^ self_sbox(r) ^ tau(S, j) ^ bit(S[(j - 1) % 8], 11)
    t0 = bit(r, 0) ^ bit(r, 12) ^ bit(r, 32) ^ b
    t1 = bit(r, 96) ^ bit(r, 107) ^ bit(r, 125) ^ b
    if t0:
        r ^= U0
    if t1:
        r ^= U1
    S[j] = r


def beat_inv(S, ins):
    for j in reversed(range(8)):
        upres_inv(S, j, ins[j])


# ---------------- mode ----------------

def pad_bits(msgbits):
    """Implementation padding (pseudocode.md (a)): M||1||0^z||1||len64, z=(64-((|M|+2)&63))&63.
    Returns the list of padded bits in implementation order (index 0 = first message bit)."""
    L = len(msgbits)
    z = (64 - ((L + 2) & 63)) & 63
    out = list(msgbits) + [1] + [0] * z + [1] + [(L >> (63 - k)) & 1 for k in range(64)]
    assert len(out) % 64 == 0
    return out


def const_stream_bit(padded_bits, idx):
    pos = (padded_bits + idx) % (8 * W)
    return bit(INIT[pos // W], pos % W)


def const_ins(padded_bits, beat_idx):
    return [const_stream_bit(padded_bits, 8 * beat_idx + j) for j in range(8)]


def message_beats(variant, pbits):
    """Yield per-beat 8-bit input lists in absorption order (implementation consumes the
    padded string from its last bit backwards)."""
    P = len(pbits)
    stream = [pbits[P - 1 - o] for o in range(P)]
    if variant == 1024:
        for x in stream:
            yield [x] * 8
    elif variant == 512:
        for i in range(0, P, 2):
            lo, hi = stream[i], stream[i + 1]          # axis_step(ctx, pending[1], pending[0])
            yield [hi, lo] * 4
    elif variant == 768:
        tail = {0: 0, 1: 64, 2: 32}[(P // 64) % 3]
        mixed = P - tail
        o, bt = 0, 0
        while o < mixed:
            if bt % 2 == 0:
                x = stream[o]
                yield [x] * 8
                o += 1
            else:
                hi, lo = stream[o + 1], stream[o]
                yield [hi, lo] * 4
                o += 2
            bt += 1
        while o < P:
            yield [stream[o]] * 8
            o += 1


def h_mu(S):
    return bit(S[6], 0) ^ bit(S[4], 0) ^ (bit(S[2], 0) & bit(S[0], 0))


def h_nu(S):
    return bit(S[7], 0) ^ bit(S[5], 0) ^ (bit(S[3], 0) & bit(S[1], 0))


def digest_phase(S, variant, padded_bits, nfin=1024):
    """Runs finalization+digest generation from state S (modified). Returns list of h bits in
    generation order h_0, h_1, ... (for 512/768: h_mu then h_nu within a beat)."""
    for i in range(nfin):
        beat(S, const_ins(padded_bits, i))
    return digest_gen(S, variant, padded_bits, nfin)


def digest_gen(S, variant, padded_bits, start_beat=1024, nbits=None):
    d = {512: 512, 768: 768, 1024: 1024}[variant]
    if nbits is None:
        nbits = d
    out = []
    bt = start_beat
    while len(out) < nbits:
        if variant == 1024 or (variant == 768 and bt % 2 == 0):
            out.append(h_mu(S))
        else:
            out.append(h_mu(S))
            out.append(h_nu(S))
        beat(S, const_ins(padded_bits, bt))
        bt += 1
    return out[:nbits]


def hbits_to_bytes(hb, variant):
    """Implementation output order: h_mu at digest bit (d-1-i) for 1024; pairs for 512/768."""
    d = len(hb)
    out = bytearray(d // 8)
    for idx, b in enumerate(hb):
        pos = d - 1 - idx
        if variant == 512 or variant == 768:
            pass
        if b:
            out[pos >> 3] |= 1 << (7 - (pos & 7))
    return bytes(out)


def axis_hash_bits(variant, msgbits, return_state=False):
    pbits = pad_bits(msgbits)
    S = list(INIT)
    for ins in message_beats(variant, pbits):
        beat(S, ins)
    Y = list(S)
    hb = digest_phase(S, variant, len(pbits))
    if variant == 1024:
        dig = hbits_to_bytes(hb, 1024)
    else:
        # 512: beat i writes h_mu at d-2-2i, h_nu at d-1-2i; 768 via same rule in pairs/singles
        d = len(hb)
        out = bytearray(d // 8)
        # reconstruct positions following axis_write_digest768_bitwise / emit_digest
        pos_list = []
        idx = 0
        bt = 1024
        while idx < d:
            if variant == 768 and bt % 2 == 0:
                pos_list.append(d - 1 - idx)
                idx += 1
            else:
                pos_list.append(d - 2 - idx)
                pos_list.append(d - 1 - idx)
                idx += 2
            bt += 1
        for b, pos in zip(hb, pos_list):
            if b:
                out[pos >> 3] |= 1 << (7 - (pos & 7))
        dig = bytes(out)
    return (dig, Y) if return_state else dig


def bytes_to_bits(msg, nbits):
    return [(msg[i >> 3] >> (7 - (i & 7))) & 1 for i in range(nbits)]


def rand_state():
    return [int.from_bytes(os.urandom(24), 'little') for _ in range(8)]
