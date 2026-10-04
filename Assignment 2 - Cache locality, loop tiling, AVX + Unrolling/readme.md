# Matrix Multiplication Optimization Experiment

This experiment compares different optimization techniques for a **1024 × 1024 floating-point matrix multiplication**.

The goal is to measure how loop transformations and SIMD optimizations improve matrix multiplication performance.

## Optimizations Implemented

### 1. Naive Matrix Multiplication

Uses the conventional `i-j-k` loop order:

```text
for i
    for j
        for k
            C[i][j] += A[i][k] * B[k][j]
```

This version is used as the baseline.

### 2. Loop Interchange

Changes the loop order from:

```text
i → j → k
```

to:

```text
i → k → j
```

This improves memory locality because accesses to `B` and `C` become more sequential.

### 3. Loop Tiling

The matrices are divided into smaller blocks of **32 × 32**.

Tiling improves cache utilization by allowing portions of the matrices to remain in the CPU cache during computation.

### 4. AVX2 Vectorization

Uses **256-bit AVX2 registers** to process **8 floats simultaneously**.

```text
256 bits / 32 bits = 8 floats
```

This provides data-level parallelism through SIMD instructions.

### 5. AVX2 + Loop Unrolling

Combines AVX2 vectorization with **4× loop unrolling**.

Loop unrolling reduces loop-control overhead and exposes more independent operations to the processor.

---

## Build

### MSVC

Compile with AVX2 and optimization enabled:

```bat
cl /O2 /arch:AVX2 /EHsc loop_tiling.cpp
```

Run:

```bat
.\loop_tiling.exe
```


Run:

```bash
./loop_tiling.exe
```

---

## Benchmark Configuration

| Parameter | Value |
|---|---:|
| Matrix size | 1024 × 1024 |
| Data type | `float` |
| Tile size | 32 × 32 |
| AVX width | 256-bit |
| Floats per AVX2 vector | 8 |
| Unrolling factor | 4 |
| Benchmark iterations | 3 |

All optimized implementations are validated against the naive implementation before benchmarking.

---

## Results

Measured on the test system:

| Implementation | Time | GFLOPS | Speedup |
|---|---:|---:|---:|
| Naive (i-j-k) | 1563.98 ms | 1.373 | 1.00× |
| Loop Interchange | 466.195 ms | 4.606 | **3.35×** |
| Loop Tiling | 581.389 ms | 3.694 | **2.69×** |
| AVX2 | 192.382 ms | 11.163 | **8.13×** |
| AVX2 + Unrolling | 207.125 ms | 10.368 | **7.55×** |

### Observations

- **Loop interchange** provided a significant improvement, achieving approximately **3.35× speedup** over the naive implementation.
- **Loop tiling** achieved approximately **2.69× speedup**. For this particular implementation and system, it was slower than the simple loop-interchanged version.
- **AVX2** provided the largest improvement, reaching approximately **8.13× speedup** over the naive implementation.
- **AVX2 + unrolling** achieved approximately **7.55× speedup**. In this benchmark, unrolling did not improve performance over the AVX2-only implementation.

```text
Naive
  │
  ├── Loop Interchange ──> Better memory locality
  │
  ├── Loop Tiling ───────> Better cache reuse
  │
  ├── AVX2 ──────────────> SIMD / data-level parallelism
  │
  └── Unrolling ─────────> More instruction-level parallelism
```
