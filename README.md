# Typemetric-JIT

> **Metric Type-Space Optimization and Geometric Diversity Pruning for JIT Compilers & Virtual Machines**

Typemetric-JIT is a zero-dependency, C99 systems library that solves polymorphic version explosion and branch dispatch latency in Lazy Basic Block Versioning (LBBV) and trace-based JIT compilers.

---

## Overview of Architectural Components

```
typemetric-jit/
├── core/              # Standalone C99 engine (Metric k-Center, Bit-Packed Keys, Trace Stitching, Spectral Fourier, Harmonic)
│   ├── include/       # Public C API (typemetric/typemetric.h, typemetric/spectral.h, typemetric/harmonic.h)
│   └── src/           # Implementation: metric_space.c, gonzalez.c, short_key.c, trace_stitch.c,
│                      #                 walsh_hadamard.c, graph_laplacian.c, chebyshev.c, phase_fourier.c,
│                      #                 harmonic.c
├── adapters/          # Target-specific VM integrations
│   ├── yjit/          # Ruby YJIT Rust FFI adapter (replaces 4-version limit)
│   ├── cpython/       # CPython 3.13+ Tier 2 micro-op (uop) trace pre-linker
│   └── minivm/        # Reference adapter for MiniVM Lua JIT
├── verification/      # Formal SMT verification suite using Microsoft Z3
│   ├── verify_metric_axioms.py
│   ├── verify_gonzalez_bound.py
│   ├── verify_lowering_soundness.py
│   ├── verify_fourier_soundness.py
│   └── verify_harmonic_soundness.py
├── benchmarks/        # PMU hardware counter harness and Ray distributed fuzzer
│   ├── perf_counters.c
│   ├── bench_gc_reaping.c
│   ├── bench_spectral_logic.c
│   ├── bench_chebyshev.c
│   ├── bench_harmonic_lsh.c
│   └── ray_distributed_fuzzer.py
└── paper/             # Formal mathematical specification and algorithmic bounds
│   └── ray_distributed_fuzzer.py
└── paper/             # Formal mathematical specification and algorithmic bounds
```

---

## 1. Core Principles

### A. The Discrete Metric Type Space $(\mathcal{T}^k, d)$
Dynamic type signatures are modeled as points in a discrete metric space satisfying non-negativity, identity of indiscernibles, symmetry, and the triangle inequality:
$$d(\vec{u}, \vec{v}) \le d(\vec{u}, \vec{w}) + d(\vec{w}, \vec{v})$$

### B. Metric $k$-Center Pruning (Gonzalez Sampling)
When a JIT compiler accumulates more versions than its code buffer capacity ($N > K$), rather than evicting via temporal access recency (LRU) which induces deoptimization thrashing under bursty workloads, Typemetric executes **Gonzalez Farthest-Point Sampling**. This achieves a proven factor-2 approximation to the optimal metric $k$-center problem in $O(K \cdot N)$ time and $O(1)$ auxiliary memory (zero heap allocations).

### C. Bit-Packed 64-bit Scalar Keys
For signatures with $\le 8$ parameters, types are bit-packed into a native 64-bit scalar register word:
$$\text{key} = \tau_0 \mid (\tau_1 \ll 8) \mid (\tau_2 \ll 16) \mid \dots$$
Version checking transforms from an indirect memory loop (`memcmp`) into a single CPU register compare instruction (`cmp rdx, [version+offset]`).

### D. Speculative Trace Stitching
Pre-links downstream branch targets matching outgoing type inferences directly into contiguous machine code during planning, eliminating cold runtime trampoline exits.

### E. Frequency-Domain Compute & Fourier Spectral Optimization
- **Walsh-Hadamard Transform (WHT)**: Transforms branchy decision trees into sparse Fourier polynomials, evaluated via SIMD bitwise parity in branchless integer arithmetic (zero CPU pipeline stalls).
- **Spectral CFG Optimization (Graph Laplacian)**: Computes the Fiedler vector from the Laplacian $L = D - A$ to order basic blocks for minimal jump displacement and near-zero instruction cache misses.
- **Chebyshev Spectral Function Compression**: Compresses 32 KB discrete lookup tables down to 56 bytes (585x compression) using Chebyshev expansions evaluated via Clenshaw's recurrence in hardware FMA registers.
- **Phase Detection**: Short-Time Fourier Transform (STFT) on CPU hardware counters to predict execution loop periodicities.

---

## 2. Formal Verification (Z3 SMT Solver)

All mathematical invariants and lowering semantics are formally proven using Microsoft Z3 in `verification/`:
- **Metric Axioms**: Proves non-negativity, identity, symmetry, and triangle inequality across all type triples (0 counterexamples).
- **Approximation Bound**: Proves the Gonzalez sampling radius $R_{\text{Gonzalez}} \le 2 \cdot R_{\text{OPT}}$ across arbitrary metric spaces.
- **Lowering Soundness**: Proves bitwise isomorphism between 64-bit scalar key comparisons and sequential `memcmp` across the full $2^{64}$ input space, and proves trap-freedom for 64-bit integer floor division and modulo on x86-64 and RV64GC.
- **Spectral Soundness**: Proves exact invertibility of the Fast Walsh-Hadamard Transform, Parseval energy conservation on Boolean cubes, and algebraic equivalence of Clenshaw's 3-term recurrence for Chebyshev polynomials.

To run proofs:
```bash
python verification/run_proofs.py
```

---

## 3. Building Core Library

The core C99 engine has zero dependencies other than standard libc:
```bash
cd core
make
```
Generates `libtypemetric.a` and `libtypemetric.so`.

---

## 4. Compiler Adapters

### Ruby YJIT (Rust)
Located in `adapters/yjit/`. Translates Ruby `VALUE` type tags into metric space representations, replaces YJIT's hardcoded 4-version ceiling with metric $k$-center diversity pruning, and injects 64-bit scalar keys into `ctx.get_local_type()` lookups.

### CPython 3.13+ Tier 2 (C)
Located in `adapters/cpython/`. Maps `PyTypeObject*` addresses (`&PyLong_Type`, `&PyFloat_Type`, etc.) to 8-bit kinds and adapts speculative trace stitching to pre-link micro-ops (uops) across basic block boundaries.

### MiniVM (Reference)
Located in `adapters/minivm/`. Reference integration demonstrating $O(1)$ block cache dispatch and version reaping in an LBBV Lua runtime.

---

## License

Licensed under either of Apache License, Version 2.0 or MIT License at your option.
