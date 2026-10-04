#include <immintrin.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

constexpr int N = 1024;

// ============================================================
// Matrix storage
// ============================================================

using Matrix = std::vector<float>;


// ============================================================
// Initialize matrix
// ============================================================

void initialize(Matrix& M)
{
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (auto& x : M)
        x = dist(gen);
}


// ============================================================
// 1. NAIVE MATRIX MULTIPLICATION
//
// Loop order:
//
//     i -> j -> k
//
// C[i][j] += A[i][k] * B[k][j]
//
// This has poor locality for B because B[k][j] accesses
// memory with a stride of N.
// ============================================================

void matmul_naive(
    const Matrix& A,
    const Matrix& B,
    Matrix& C)
{
    std::fill(C.begin(), C.end(), 0.0f);

    for (int i = 0; i < N; ++i) {

        for (int j = 0; j < N; ++j) {

            float sum = 0.0f;

            for (int k = 0; k < N; ++k) {

                sum +=
                    A[i * N + k] *
                    B[k * N + j];
            }

            C[i * N + j] = sum;
        }
    }
}


// ============================================================
// 2. LOOP INTERCHANGE
//
// Original:
//
//     for i
//         for j
//             for k
//
// New:
//
//     for i
//         for k
//             for j
//
// The important change is that j becomes the innermost loop.
//
// B[k][j] -> sequential access
// C[i][j] -> sequential access
//
// This improves spatial locality significantly.
// ============================================================

void matmul_loop_interchange(
    const Matrix& A,
    const Matrix& B,
    Matrix& C)
{
    std::fill(C.begin(), C.end(), 0.0f);

    for (int i = 0; i < N; ++i) {

        for (int k = 0; k < N; ++k) {

            float a =
                A[i * N + k];

            for (int j = 0; j < N; ++j) {

                C[i * N + j] +=
                    a * B[k * N + j];
            }
        }
    }
}


// ============================================================
// 3. LOOP TILING / BLOCKING
//
// Divide matrices into TILE x TILE blocks.
//
// Loop structure:
//
//     ii
//       kk
//         jj
//           i
//             k
//               j
//
// This keeps portions of A, B and C in cache.
// ============================================================

void matmul_tiled(
    const Matrix& A,
    const Matrix& B,
    Matrix& C)
{
    constexpr int TILE = 32;

    std::fill(C.begin(), C.end(), 0.0f);

    for (int ii = 0; ii < N; ii += TILE) {

        for (int kk = 0; kk < N; kk += TILE) {

            for (int jj = 0; jj < N; jj += TILE) {

                int i_end =
                    std::min(ii + TILE, N);

                int k_end =
                    std::min(kk + TILE, N);

                int j_end =
                    std::min(jj + TILE, N);


                for (int i = ii;
                     i < i_end;
                     ++i) {

                    for (int k = kk;
                         k < k_end;
                         ++k) {

                        float a =
                            A[i * N + k];

                        for (int j = jj;
                             j < j_end;
                             ++j) {

                            C[i * N + j] +=
                                a *
                                B[k * N + j];
                        }
                    }
                }
            }
        }
    }
}


// ============================================================
// 4. AVX2
//
// Process 8 floats simultaneously.
//
// 256-bit AVX register:
//
//     256 bits / 32 bits = 8 floats
//
// C[i][j:j+8] += A[i][k] * B[k][j:j+8]
// ============================================================

void matmul_avx2(
    const Matrix& A,
    const Matrix& B,
    Matrix& C)
{
    std::fill(C.begin(), C.end(), 0.0f);

    for (int i = 0; i < N; ++i) {

        for (int j = 0;
             j < N;
             j += 8) {

            __m256 sum =
                _mm256_setzero_ps();

            for (int k = 0;
                 k < N;
                 ++k) {

                __m256 a =
                    _mm256_set1_ps(
                        A[i * N + k]);

                __m256 b =
                    _mm256_loadu_ps(
                        &B[k * N + j]);

                sum =
                    _mm256_add_ps(
                        sum,
                        _mm256_mul_ps(a, b));
            }

            _mm256_storeu_ps(
                &C[i * N + j],
                sum);
        }
    }
}


// ============================================================
// 5. AVX2 + LOOP UNROLLING
//
// Unroll k loop by 4.
//
// Instead of:
//
//     k++
//
// perform:
//
//     k
//     k+1
//     k+2
//     k+3
//
// This reduces loop-control overhead and exposes more
// independent operations to the CPU.
// ============================================================

void matmul_avx2_unrolled(
    const Matrix& A,
    const Matrix& B,
    Matrix& C)
{
    std::fill(C.begin(), C.end(), 0.0f);

    for (int i = 0; i < N; ++i) {

        for (int j = 0;
             j < N;
             j += 8) {

            __m256 sum =
                _mm256_setzero_ps();

            int k = 0;


            // ------------------------------------------------
            // Unroll by 4
            // ------------------------------------------------

            for (; k <= N - 4; k += 4) {

                __m256 a0 =
                    _mm256_set1_ps(
                        A[i * N + k]);

                __m256 b0 =
                    _mm256_loadu_ps(
                        &B[k * N + j]);

                sum =
                    _mm256_add_ps(
                        sum,
                        _mm256_mul_ps(a0, b0));


                __m256 a1 =
                    _mm256_set1_ps(
                        A[i * N + k + 1]);

                __m256 b1 =
                    _mm256_loadu_ps(
                        &B[(k + 1) * N + j]);

                sum =
                    _mm256_add_ps(
                        sum,
                        _mm256_mul_ps(a1, b1));


                __m256 a2 =
                    _mm256_set1_ps(
                        A[i * N + k + 2]);

                __m256 b2 =
                    _mm256_loadu_ps(
                        &B[(k + 2) * N + j]);

                sum =
                    _mm256_add_ps(
                        sum,
                        _mm256_mul_ps(a2, b2));


                __m256 a3 =
                    _mm256_set1_ps(
                        A[i * N + k + 3]);

                __m256 b3 =
                    _mm256_loadu_ps(
                        &B[(k + 3) * N + j]);

                sum =
                    _mm256_add_ps(
                        sum,
                        _mm256_mul_ps(a3, b3));
            }


            // ------------------------------------------------
            // Remaining iterations
            // ------------------------------------------------

            for (; k < N; ++k) {

                __m256 a =
                    _mm256_set1_ps(
                        A[i * N + k]);

                __m256 b =
                    _mm256_loadu_ps(
                        &B[k * N + j]);

                sum =
                    _mm256_add_ps(
                        sum,
                        _mm256_mul_ps(a, b));
            }


            _mm256_storeu_ps(
                &C[i * N + j],
                sum);
        }
    }
}


// ============================================================
// Validation
// ============================================================

bool compare_matrices(
    const Matrix& A,
    const Matrix& B)
{
    for (size_t i = 0;
         i < A.size();
         ++i) {

        if (std::fabs(A[i] - B[i]) > 1e-2f) {

            std::cerr
                << "Mismatch at index "
                << i
                << ": "
                << A[i]
                << " vs "
                << B[i]
                << '\n';

            return false;
        }
    }

    return true;
}


// ============================================================
// Benchmark helper
// ============================================================

template <typename Func>
double benchmark(
    const char* name,
    Func func,
    const Matrix& A,
    const Matrix& B,
    Matrix& C,
    const Matrix& reference)
{
    // --------------------------------------------------------
    // Warm-up
    // --------------------------------------------------------

    func(A, B, C);


    // --------------------------------------------------------
    // Validate
    // --------------------------------------------------------

    if (!compare_matrices(C, reference)) {

        std::cerr
            << name
            << " FAILED validation\n";

        return -1.0;
    }


    // --------------------------------------------------------
    // Benchmark
    // --------------------------------------------------------

    constexpr int ITERATIONS = 3;

    auto start =
        std::chrono::high_resolution_clock::now();


    for (int i = 0;
         i < ITERATIONS;
         ++i) {

        func(A, B, C);
    }


    auto end =
        std::chrono::high_resolution_clock::now();


    double milliseconds =
        std::chrono::duration<double, std::milli>(
            end - start
        ).count();


    milliseconds /= ITERATIONS;


    // --------------------------------------------------------
    // FLOPS
    //
    // Matrix multiplication:
    //
    //     N^3 multiplications
    //     N^3 additions
    //
    // Approximately:
    //
    //     2 * N^3 FLOPs
    // --------------------------------------------------------

    double flops =
        2.0 *
        N *
        N *
        N;


    double gflops =
        flops /
        (milliseconds * 1e6);


    std::cout
        << std::left
        << std::setw(25)
        << name

        << std::right
        << std::setw(12)
        << milliseconds

        << " ms   "

        << std::setw(8)
        << gflops

        << " GFLOPS\n";


    return milliseconds;
}


// ============================================================
// Main
// ============================================================

int main()
{
    std::cout
        << "Matrix Multiplication Optimization Benchmark\n";

    std::cout
        << "Matrix size: "
        << N
        << " x "
        << N
        << "\n\n";


    Matrix A(N * N);
    Matrix B(N * N);

    Matrix C(N * N);
    Matrix reference(N * N);


    initialize(A);
    initialize(B);


    // --------------------------------------------------------
    // Generate reference
    // --------------------------------------------------------

    std::cout
        << "Generating reference result...\n";

    matmul_naive(
        A,
        B,
        reference);


    // --------------------------------------------------------
    // Benchmark
    // --------------------------------------------------------

    std::cout
        << "\nResults:\n";

    std::cout
        << "-------------------------------------------------------------\n";

    std::cout
        << std::left
        << std::setw(25)
        << "Implementation"
        << std::right
        << std::setw(12)
        << "Time"
        << "   "
        << std::setw(8)
        << "GFLOPS"
        << '\n';

    std::cout
        << "-------------------------------------------------------------\n";


    double naive =
        benchmark(
            "Naive (i-j-k)",
            matmul_naive,
            A,
            B,
            C,
            reference);


    double interchange =
        benchmark(
            "Loop Interchange",
            matmul_loop_interchange,
            A,
            B,
            C,
            reference);


    double tiled =
        benchmark(
            "Loop Tiling",
            matmul_tiled,
            A,
            B,
            C,
            reference);


    double avx =
        benchmark(
            "AVX2",
            matmul_avx2,
            A,
            B,
            C,
            reference);


    double unrolled =
        benchmark(
            "AVX2 + Unrolling",
            matmul_avx2_unrolled,
            A,
            B,
            C,
            reference);


    std::cout
        << "-------------------------------------------------------------\n";


    // --------------------------------------------------------
    // Speedup
    // --------------------------------------------------------

    std::cout
        << "\nSpeedup vs Naive:\n";

    std::cout
        << "-------------------------------------------------------------\n";


    if (naive > 0.0) {

        std::cout
            << std::left
            << std::setw(25)
            << "Loop Interchange"
            << std::right
            << std::setw(8)
            << naive / interchange
            << "x\n";


        std::cout
            << std::left
            << std::setw(25)
            << "Loop Tiling"
            << std::right
            << std::setw(8)
            << naive / tiled
            << "x\n";


        std::cout
            << std::left
            << std::setw(25)
            << "AVX2"
            << std::right
            << std::setw(8)
            << naive / avx
            << "x\n";


        std::cout
            << std::left
            << std::setw(25)
            << "AVX2 + Unrolling"
            << std::right
            << std::setw(8)
            << naive / unrolled
            << "x\n";
    }


    std::cout
        << "-------------------------------------------------------------\n";


    return 0;
}
