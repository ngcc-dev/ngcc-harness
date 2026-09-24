import sys

# Map published Galas instances to NGCC instances for each secpar.
REPLACE_MAP = {
    "galas::galas_128s": None,  # s128 is not an NGCC instance
    "galas::galas_128f": None,
    "galas::galas_192s": None,  # s192 is not an NGCC instance
    "galas::galas_192f": None,
    "galas::galas_256s": ["galas::galas_ngcc_256s"],
    "galas::galas_256f": ["galas::galas_ngcc_256f"],
}

# For 384/512: there are no published galas_384/512 instances to copy from.
# We need to add them manually by pattern from the 256 lines.

for fn in ["faest.cpp", "vole_commit.cpp", "owf_proof.cpp", "small_vole.cpp"]:
    with open(fn) as f:
        lines = f.readlines()
    out = []
    for line in lines:
        out.append(line)
        if "template" not in line:
            continue
        # After galas_256s/f lines, add ngcc_256s/f and ngcc_384/512.
        if "galas::galas_256s" in line and "ngcc" not in line:
            out.append(line.replace("galas::galas_256s", "galas::galas_ngcc_256s"))
            # Add 384s and 512s variants
            for sec, suffix in [("384", "384s"), ("512", "512s")]:
                new_line = line.replace("galas::galas_256s", f"galas::galas_ngcc_{suffix}")
                new_line = new_line.replace("secpar::s256", f"secpar::s{sec}")
                out.append(new_line)
        elif "galas::galas_256f" in line and "ngcc" not in line:
            out.append(line.replace("galas::galas_256f", "galas::galas_ngcc_256f"))
            for sec, suffix in [("384", "384f"), ("512", "512f")]:
                new_line = line.replace("galas::galas_256f", f"galas::galas_ngcc_{suffix}")
                new_line = new_line.replace("secpar::s256", f"secpar::s{sec}")
                out.append(new_line)
    with open(fn, "w") as f:
        f.writelines(out)
    ngcc = sum(1 for l in out if "ngcc" in l)
    print(fn, "lines:", len(out), "ngcc:", ngcc)
