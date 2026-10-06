#!/usr/bin/env python3
"""Check sign-29-7: all submitted Tins child-node expansions omit the parent seed."""

import argparse
import hashlib
from pathlib import Path
import zipfile


ROOT = Path(__file__).resolve().parent / "Implementations"
PINNED_SHA256 = "84affc1f7cbdeec48a7cb6df3349b5708644672f2ac12140c529e74f6d57ca21"
parser = argparse.ArgumentParser()
parser.add_argument("--archive", type=Path, help="optional pinned official Tins.zip")
args = parser.parse_args()
archive = None
if args.archive:
    data = args.archive.read_bytes()
    assert hashlib.sha256(data).hexdigest() == PINNED_SHA256, "unexpected Tins archive"
    archive = zipfile.ZipFile(args.archive)

for family, suffix in (
    ("Reference_Implementation", ""),
    ("Optimized_Implementation", "_opt"),
    ("Additional_Implementation", "_add"),
):
    for level in (128, 256, 512):
        name = f"Tins{level}{suffix}"
        relative = f"{family}/{name}/bavc_commit.c"
        source = ((ROOT / relative).read_text() if archive is None else
                  archive.read("tins/Implementations/" + relative).decode())
        start = source.index("void ChildNodeGen(")
        end = source.index("void bavc_commit(", start)
        body = source[start:end].split("{", 1)[1]
        assert "rseed" not in body, name
        assert "memcpy(msg, salt, RSEED_SIZE);" in body, name
        assert body.count("pseudoXOF(NODE_SIZE * 8, msg, RSEED_SIZE * 8,") == 2, name
        assert "aux[e][j] = alpha[j] ^ alpha_acc[j];" in source, name
        assert "aux[e][j + N_TUPLE_SIZE] = beta[j] ^ beta_acc[j];" in source, name
        print(f"PASS {name}: public-salt children and XOR-masked witness")

if archive is not None:
    archive.close()
