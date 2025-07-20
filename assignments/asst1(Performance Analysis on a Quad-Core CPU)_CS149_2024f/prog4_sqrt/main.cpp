#include <immintrin.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>

#include <algorithm>

#include "CycleTimer.h"
#include "sqrt_ispc.h"

using namespace ispc;

extern void sqrtSerial(int N, float startGuess, float* values, float* output);

static void verifyResult(int N, float* result, float* gold) {
    for (int i = 0; i < N; i++) {
        if (fabs(result[i] - gold[i]) > 1e-4) {
            printf("Error: [%d] Got %f expected %f\n", i, result[i], gold[i]);
        }
    }
}

void sqrt_my_avx2(int N, float initialGuess, float* values, float* output) {
    static const float kThreshold = 0.00001f;
    const __m256 initialGuess_v = _mm256_set1_ps(initialGuess);
    const __m256 kThreshold_v = _mm256_set1_ps(kThreshold);
    const __m256 one_v = _mm256_set1_ps(1.f);
    const __m256 three_v = _mm256_set1_ps(3.f);
    const __m256 half_v = _mm256_set1_ps(0.5f);

    // 做与操作，实现取绝对值
    const __m256 abs_mask_v =
        _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));

    for (int i = 0; i <= N - 8; i += 8) {
        __m256 x = _mm256_loadu_ps(values + i);
        __m256 guess = initialGuess_v;

        __m256 continue_mask;

        while (true) {
            __m256 guess_sq = _mm256_mul_ps(guess, guess);
            __m256 term = _mm256_mul_ps(guess_sq, x);
            __m256 error =
                _mm256_and_ps((_mm256_sub_ps(term, one_v)), abs_mask_v);

            continue_mask = _mm256_cmp_ps(error, kThreshold_v, _CMP_GT_OQ);

            if (_mm256_movemask_ps(continue_mask) == 0) {
                break;
            }

            __m256 guess_cubed = _mm256_mul_ps(guess_sq, guess);
            __m256 term2 = _mm256_mul_ps(x, guess_cubed);
            __m256 term3 = _mm256_mul_ps(three_v, guess);
            __m256 new_guess_unscaled = _mm256_sub_ps(term3, term2);
            __m256 new_guess = _mm256_mul_ps(new_guess_unscaled, half_v);

            guess = _mm256_blendv_ps(guess, new_guess, continue_mask);
        }

        __m256 result = _mm256_mul_ps(x, guess);
        _mm256_storeu_ps(output + i, result);
    }
}

int main() {
    const unsigned int N = 20 * 1000 * 1000;
    const float initialGuess = 1.0f;

    float* values = new float[N];
    float* output = new float[N];
    float* gold = new float[N];

    for (unsigned int i = 0; i < N; i++) {
        // TODO: CS149 students.  Attempt to change the values in the
        // array here to meet the instructions in the handout: we want
        // to you generate best and worse-case speedups

        // starter code populates array with random input values
        values[i] = .001f + 2.998f * static_cast<float>(rand()) / RAND_MAX;
        // values[i] = 1.f;
        // values[i] = 2.999f;
        // values[i] = ((i % 8) ? 1.f : 2.999f);
    }

    // generate a gold version to check results
    for (unsigned int i = 0; i < N; i++) gold[i] = sqrt(values[i]);

    //
    // And run the serial implementation 3 times, again reporting the
    // minimum time.
    //
    double minSerial = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        sqrtSerial(N, initialGuess, values, output);
        double endTime = CycleTimer::currentSeconds();
        minSerial = std::min(minSerial, endTime - startTime);
    }

    printf("[sqrt serial]:\t\t[%.3f] ms\n", minSerial * 1000);

    verifyResult(N, output, gold);

    //
    // Compute the image using the ispc implementation; report the minimum
    // time of three runs.
    //
    double minISPC = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        sqrt_ispc(N, initialGuess, values, output);
        double endTime = CycleTimer::currentSeconds();
        minISPC = std::min(minISPC, endTime - startTime);
    }

    printf("[sqrt ispc]:\t\t[%.3f] ms\n", minISPC * 1000);

    verifyResult(N, output, gold);

    // Clear out the buffer
    for (unsigned int i = 0; i < N; ++i) output[i] = 0;

    // My version of AVX2

    double minMyAVX2 = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        sqrt_my_avx2(N, initialGuess, values, output);
        double endTime = CycleTimer::currentSeconds();
        minMyAVX2 = std::min(minMyAVX2, endTime - startTime);
    }

    printf("[sqrt my avx2]:\t\t[%.3f] ms\n", minMyAVX2 * 1000);

    verifyResult(N, output, gold);

    // Clear out the buffer
    for (unsigned int i = 0; i < N; ++i) output[i] = 0;

    //
    // Tasking version of the ISPC code
    //
    double minTaskISPC = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        sqrt_ispc_withtasks(N, initialGuess, values, output);
        double endTime = CycleTimer::currentSeconds();
        minTaskISPC = std::min(minTaskISPC, endTime - startTime);
    }

    printf("[sqrt task ispc]:\t[%.3f] ms\n", minTaskISPC * 1000);

    verifyResult(N, output, gold);

    printf("\t\t\t\t(%.2fx speedup from ISPC)\n", minSerial / minISPC);
    printf("\t\t\t\t(%.2fx speedup from My AVX2)\n", minSerial / minMyAVX2);
    printf("\t\t\t\t(%.2fx speedup from task ISPC)\n", minSerial / minTaskISPC);

    delete[] values;
    delete[] output;
    delete[] gold;

    return 0;
}
