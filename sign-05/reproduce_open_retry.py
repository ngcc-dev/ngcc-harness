#!/usr/bin/env python3
"""Check the sign-05-3 non-progress branch in Chinith's signing pseudocode."""

def main() -> int:
    # SM4th-d3-128s-lo, Tables 1 and 3 and BAVC.PosInTree.
    tau, tree_depth, total_leaves, topen = 11, 11, 22_528, 102
    indices = tuple(range(0, 1100, 100))
    assert len(indices) == tau and all(index < 1 << tree_depth for index in indices)
    leaves = [total_leaves - 1 + tau * index + layer
              for layer, index in enumerate(indices)]

    paths: set[int] = set()
    for node in leaves:
        while True:
            paths.add(node)
            if node == 0:
                break
            node = (node - 1) // 2
    opening_seeds = len(paths) - 2 * tau + 1
    assert len(paths) == 135 and opening_seeds == 114 > topen

    print(f"leaf_positions={','.join(map(str, leaves))}")
    print(f"path_union_nodes={len(paths)}")
    print(f"opening_seeds={opening_seeds} limit={topen}")
    print("CHINITH_FIXED_COUNTER_RETRY=CONFIRMED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
