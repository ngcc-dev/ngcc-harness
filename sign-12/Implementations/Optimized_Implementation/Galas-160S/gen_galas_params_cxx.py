import json, subprocess

def to_limbs(val, nw):
    return [(val >> (64*i)) & ((1<<64)-1) for i in range(nw)]

def to_bytes(val, nb):
    return [(val >> (8*i)) & 0xff for i in range(nb)]

subprocess.run(["git", "show", "HEAD:submission/optimized/galas_params_generated.hpp"],
               stdout=open("/tmp/orig_params.hpp", "wb"), check=True)
with open("/tmp/orig_params.hpp") as f:
    content = f.read().rstrip()
# strip closing
idx = content.rfind("} // namespace faest")
if idx > 0:
    content = content[:idx].rstrip()

for n in [384, 512]:
    d = json.load(open(f"params/generated/galas_{n}.json"))
    nw, nb = n//64, n//8
    L = ["", "template <>", "struct galas_params<secpar::s%d> {" % n]
    L.append("    static constexpr std::size_t n = %d;" % n)
    L.append("    static constexpr std::size_t nb = %d;" % nb)
    L.append("    static constexpr std::size_t nw = %d;" % nw)
    L.append("    static constexpr std::size_t m = %d;" % (n//2))
    L.append("")
    for c in ("c0","c1","c2"):
        bs = to_bytes(int(d[c],16), nb)
        hx = ",".join("0x%02X" % b for b in bs)
        L.append("    static constexpr std::array<uint8_t, %d> %s = {%s};" % (nb, c, hx))
    L.append("")
    for mn in ("M0","M1","M2","L0","L1","L2","L3"):
        coeffs = [int(x,16) for x in d[mn]]
        L.append("    static constexpr std::array<std::array<std::uint64_t, %d>, %d> %s = {{" % (nw, n, mn))
        for cv in coeffs:
            limbs = to_limbs(cv, nw)
            lh = ",".join("0x%016XULL" % l for l in limbs)
            L.append("        { %s }," % lh)
        L.append("    }};")
        L.append("")
    bp = d["Bprime"]
    L.append("    static constexpr uint8_t GALAS_BPRIME_%d[%d][%d] = {" % (n, len(bp), nb))
    for b in bp:
        bs = to_bytes(int(b,16), nb)
        L.append("        {%s}," % ",".join("0x%02X" % x for x in bs))
    L.append("    };")
    P = d["P"]
    L.append("    static constexpr uint16_t GALAS_P_%d[%d] = {" % (n, len(P)))
    for i in range(0,len(P),16):
        L.append("    " + ",".join(str(x) for x in P[i:i+16]) + ",")
    L.append("    };")
    L.append("};")
    content += "\n" + "\n".join(L)

content += "\n} // namespace faest\n\n#endif\n"
with open("submission/optimized/galas_params_generated.hpp","w") as f:
    f.write(content)
print("done, %d lines" % content.count("\n"))
