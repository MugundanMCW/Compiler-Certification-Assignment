# Compiler Certification Assignments

This repository contains assignments completed as part of the **Compiler Certification** program.

The assignments focus on LLVM IR optimization, compiler transformations, cache locality, loop optimizations, SIMD vectorization, and performance analysis.

## Assignments

| Assignment | Description |
|---|---|
| [Assignment 1 - LLVM IR Opt Passes](https://github.com/MugundanMCW/Compiler-Certification-Assignment/tree/main/Assignment%201%20-%20LLVM%20IR%20Opt%20Passes) | Implementation of custom LLVM IR optimization passes |
| [Assignment 2 - Cache Locality, Loop Tiling, AVX + Unrolling](https://github.com/MugundanMCW/Compiler-Certification-Assignment/tree/main/Assignment%202%20-%20Cache%20locality%2C%20loop%20tiling%2C%20AVX%20%2B%20Unrolling) | Study and performance evaluation of loop transformations, cache locality, SIMD vectorization, and loop unrolling |

---

## Assignment 1 - LLVM IR Optimization Passes

This assignment focuses on implementing custom optimization passes using the LLVM pass infrastructure.

### Optimizations Implemented

- **Constant Folding**
- **Strength Reduction**
- **Common Subexpression Elimination (CSE)**
- **Dead Code Elimination (DCE)**


### Implementation

The optimization passes are implemented in `HelloWorld.cpp` and executed as part of a custom LLVM pass.

The transformations operate directly on LLVM IR instructions to identify opportunities for simplification and optimization.

---

## Assignment 2 - Cache Locality, Loop Tiling, AVX + Unrolling

This assignment studies the impact of different loop and hardware-level optimizations on matrix multiplication performance.

The benchmark performs multiplication of two **1024 × 1024 floating-point matrices**.

### Optimizations Implemented

1. **Naive Matrix Multiplication**
   - Baseline `i-j-k` loop ordering.

2. **Loop Interchange**
   - Changes the loop ordering from `i-j-k` to `i-k-j`.
   - Improves memory access locality.

3. **Loop Tiling**
   - Uses **32 × 32 tiles**.
   - Improves cache reuse by operating on smaller matrix blocks.

4. **AVX2 Vectorization**
   - Uses 256-bit AVX2 registers.
   - Processes 8 `float` values simultaneously.

5. **Loop Unrolling**
   - Uses 4× loop unrolling.
   - Reduces loop-control overhead and exposes additional instruction-level parallelism.
