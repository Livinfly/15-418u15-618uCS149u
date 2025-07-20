#include <immintrin.h>
#include <stdio.h>

#include <algorithm>

#include "CycleTimer.h"
#include "saxpy_ispc.h"

extern void saxpySerial(int N, float a, float* X, float* Y, float* result);

// return GB/s
static float toBW(int bytes, float sec) {
    return static_cast<float>(bytes) / (1024. * 1024. * 1024.) / sec;
}

static float toGFLOPS(int ops, float sec) {
    return static_cast<float>(ops) / 1e9 / sec;
}

static void verifyResult(int N, float* result, float* gold) {
    for (int i = 0; i < N; i++) {
        if (result[i] != gold[i]) {
            printf("Error: [%d] Got %f expected %f\n", i, result[i], gold[i]);
        }
    }
}

static void saxpy_avx2(int N, float scale, float* X, float* Y, float* result) {
    const __m256 scale_v = _mm256_set1_ps(scale);

    int i = 0;
    for (; i <= N - 8; i += 8) {
        __m256 x = _mm256_load_ps(X + i), y = _mm256_load_ps(Y + i);
        __m256 res = _mm256_add_ps(_mm256_mul_ps(scale_v, x), y);
        _mm256_stream_ps(result + i, res);
    }
    for (; i < N; i++) {
        result[i] = scale * X[i] + Y[i];
    }
}

using namespace ispc;

int main() {
    const unsigned int N = 20 * 1000 * 1000;  // 20 M element vectors (~80 MB)
    const unsigned int TOTAL_BYTES = 4 * N * sizeof(float);
    const unsigned int TOTAL_FLOPS = 2 * N;

    float scale = 2.f;

    // float* arrayX = new float[N];
    // float* arrayY = new float[N];
    // float* resultSerial = new float[N];
    // float* resultISPC = new float[N];
    // float* resultTasks = new float[N];
    // float* resultAVX2 = new float[N];

    const int ALIGNMENT = 32;

    float* arrayX = (float*)_mm_malloc(N * sizeof(float), ALIGNMENT);
    float* arrayY = (float*)_mm_malloc(N * sizeof(float), ALIGNMENT);
    float* resultSerial = (float*)_mm_malloc(N * sizeof(float), ALIGNMENT);
    float* resultISPC = (float*)_mm_malloc(N * sizeof(float), ALIGNMENT);
    float* resultTasks = (float*)_mm_malloc(N * sizeof(float), ALIGNMENT);
    float* resultAVX2 = (float*)_mm_malloc(N * sizeof(float), ALIGNMENT);

    // initialize array values
    for (unsigned int i = 0; i < N; i++) {
        arrayX[i] = i;
        arrayY[i] = i;
        resultSerial[i] = 0.f;
        resultAVX2[i] = 0.f;
        resultISPC[i] = 0.f;
        resultTasks[i] = 0.f;
    }

    //
    // Run the serial implementation. Repeat three times for robust
    // timing.
    //
    double minSerial = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        saxpySerial(N, scale, arrayX, arrayY, resultSerial);
        double endTime = CycleTimer::currentSeconds();
        minSerial = std::min(minSerial, endTime - startTime);
    }

    printf("[saxpy serial]:\t\t[%.3f] ms\t[%.3f] GB/s\t[%.3f] GFLOPS\n",
           minSerial * 1000, toBW(TOTAL_BYTES, minSerial),
           toGFLOPS(TOTAL_FLOPS, minSerial));

    // My AVX2 version

    double minAVX2 = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        saxpy_avx2(N, scale, arrayX, arrayY, resultAVX2);
        double endTime = CycleTimer::currentSeconds();
        minAVX2 = std::min(minAVX2, endTime - startTime);
    }

    verifyResult(N, resultAVX2, resultSerial);

    printf("[saxpy avx2]:\t\t[%.3f] ms\t[%.3f] GB/s\t[%.3f] GFLOPS\n",
           minAVX2 * 1000, toBW(TOTAL_BYTES, minAVX2),
           toGFLOPS(TOTAL_FLOPS, minAVX2));

    //
    // Run the ISPC (single core) implementation
    //
    double minISPC = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        saxpy_ispc(N, scale, arrayX, arrayY, resultISPC);
        double endTime = CycleTimer::currentSeconds();
        minISPC = std::min(minISPC, endTime - startTime);
    }

    verifyResult(N, resultISPC, resultSerial);

    printf("[saxpy ispc]:\t\t[%.3f] ms\t[%.3f] GB/s\t[%.3f] GFLOPS\n",
           minISPC * 1000, toBW(TOTAL_BYTES, minISPC),
           toGFLOPS(TOTAL_FLOPS, minISPC));

    //
    // Run the ISPC (multi-core) implementation
    //
    double minTaskISPC = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        saxpy_ispc_withtasks(N, scale, arrayX, arrayY, resultTasks);
        double endTime = CycleTimer::currentSeconds();
        minTaskISPC = std::min(minTaskISPC, endTime - startTime);
    }

    verifyResult(N, resultTasks, resultSerial);

    printf("[saxpy task ispc]:\t[%.3f] ms\t[%.3f] GB/s\t[%.3f] GFLOPS\n",
           minTaskISPC * 1000, toBW(TOTAL_BYTES, minTaskISPC),
           toGFLOPS(TOTAL_FLOPS, minTaskISPC));

    printf("\t\t\t\t(%.2fx speedup from My AVX2)\n", minSerial / minAVX2);
    printf("\t\t\t\t(%.2fx speedup from use of tasks)\n",
           minISPC / minTaskISPC);
    printf("\t\t\t\t(%.2fx speedup from ISPC)\n", minSerial / minISPC);
    printf("\t\t\t\t(%.2fx speedup from task ISPC)\n", minSerial / minTaskISPC);

    // delete[] arrayX;
    // delete[] arrayY;
    // delete[] resultSerial;
    // delete[] resultISPC;
    // delete[] resultTasks;
    _mm_free(arrayX);
    _mm_free(arrayY);
    _mm_free(resultAVX2);
    _mm_free(resultSerial);
    _mm_free(resultISPC);
    _mm_free(resultTasks);

    return 0;
}
