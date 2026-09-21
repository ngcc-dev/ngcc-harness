# LOOM-AKE — LoomKEX-512

Authenticated key exchange over **Weaver KEM** ([../kem](../kem)) and **Shuttle SIG** ([../sig](../sig)). Public API: [api.h](api.h).

## Build (this instance)

From `Implementations/Reference_Implementation`:

```bash
mkdir -p build && cd build
cmake .. && cmake --build . -j$(nproc)
./bin/KAT_KEX_LoomKEX-512
./bin/test_kex_state_LoomKEX-512
```

## Parameter set

| LOOM_MODE | Weaver | Shuttle | SS (B) |
|-----------|--------|---------|--------|
| 5 | WEAVER-512 (n=512, k=4, q=7681) | SHUTTLE-512 | 64 |

See [../../../README.md](../../../README.md) for full wire sizes and KAT verification.
