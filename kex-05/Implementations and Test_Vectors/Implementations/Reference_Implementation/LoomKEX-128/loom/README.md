# LOOM-AKE — LoomKEX-128

Authenticated key exchange over **Weaver KEM** ([../kem](../kem)) and **Shuttle SIG** ([../sig](../sig)). Public API: [api.h](api.h).

## Build (this instance)

From `Implementations/Reference_Implementation`:

```bash
mkdir -p build && cd build
cmake .. && cmake --build . -j$(nproc)
./bin/KAT_KEX_LoomKEX-128
./bin/test_kex_state_LoomKEX-128
```

## Parameter set

| LOOM_MODE | Weaver | Shuttle | SS (B) |
|-----------|--------|---------|--------|
| 1 | WEAVER-128 (n=128, k=5, q=3329) | SHUTTLE-128 | 16 |

See [../../../README.md](../../../README.md) for full wire sizes and KAT verification.
