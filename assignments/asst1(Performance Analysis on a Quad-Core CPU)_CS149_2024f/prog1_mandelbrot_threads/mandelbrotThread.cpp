#include <stdio.h>

#include <thread>

#include "CycleTimer.h"

typedef struct {
    float x0, x1;
    float y0, y1;
    unsigned int width;
    unsigned int height;
    int maxIterations;
    int* output;
    int threadId;
    int numThreads;
} WorkerArgs;

extern void mandelbrotSerial(float x0, float y0, float x1, float y1, int width,
                             int height, int startRow, int numRows,
                             int maxIterations, int output[]);

//
// workerThreadStart --
//
// Thread entrypoint.
void workerThreadStart(WorkerArgs* const args) {
    // TODO FOR CS149 STUDENTS: Implement the body of the worker
    // thread here. Each thread should make a call to mandelbrotSerial()
    // to compute a part of the output image.  For example, in a
    // program that uses two threads, thread 0 could compute the top
    // half of the image and thread 1 could compute the bottom half.

    // printf("Hello world from thread %d\n", args->threadId);
    double startTime = CycleTimer::currentSeconds();

    constexpr unsigned int CHUNK_SIZE = 16;

    for (unsigned int startRow = args->threadId * CHUNK_SIZE;
         startRow < args->height; startRow += args->numThreads * CHUNK_SIZE) {
        int numRows = std::min(CHUNK_SIZE, args->height - startRow);
        mandelbrotSerial(args->x0, args->y0, args->x1, args->y1, args->width,
                         args->height, startRow, numRows, args->maxIterations,
                         args->output);
    }

    // thread 8 => 5.8x
    // int startRow = 0, nowRow = 0, tot = 0;
    // for (int i = 0; i < args->numThreads; i++) {
    //     int j = std::max(i + 1, args->numThreads - i);
    //     tot += j * j;
    //     if (i == args->threadId - 1)
    //         startRow = tot;
    //     else if (i == args->threadId)
    //         nowRow = tot;
    // }
    // double perThread = static_cast<double>(args->height) / tot;
    // startRow = static_cast<int>(startRow * perThread);
    // nowRow = static_cast<int>(nowRow * perThread);
    // int numRows = nowRow - startRow;
    // if (args->threadId == args->numThreads - 1)
    //     numRows = args->height - startRow;
    // mandelbrotSerial(args->x0, args->y0, args->x1, args->y1, args->width,
    //                  args->height, startRow, numRows, args->maxIterations,
    //                  args->output);

    // 4.x
    // int perThread = (args->height - 1) / args->numThreads + 1;
    // int startRow = args->threadId * perThread,
    //     numRows =
    //         std::min(perThread, static_cast<int>(args->height) - startRow);

    // printf("width: %d, height: %d, startRow: %d, numRows: %d\n", args->width,
    //        args->height, startRow, numRows);
    // mandelbrotSerial(args->x0, args->y0, args->x1, args->y1, args->width,
    //                  args->height, startRow, numRows, args->maxIterations,
    //                  args->output);

    double endTime = CycleTimer::currentSeconds();
    printf("NumThread: %d, Thread: %d, Time: %.3lf ms\n", args->numThreads,
           args->threadId, endTime - startTime);
}

//
// MandelbrotThread --
//
// Multi-threaded implementation of mandelbrot set image generation.
// Threads of execution are created by spawning std::threads.
void mandelbrotThread(int numThreads, float x0, float y0, float x1, float y1,
                      int width, int height, int maxIterations, int output[]) {
    static constexpr int MAX_THREADS = 32;

    if (numThreads > MAX_THREADS) {
        fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
        exit(1);
    }

    // Creates thread objects that do not yet represent a thread.
    std::thread workers[MAX_THREADS];
    WorkerArgs args[MAX_THREADS];

    for (int i = 0; i < numThreads; i++) {
        // TODO FOR CS149 STUDENTS: You may or may not wish to modify
        // the per-thread arguments here.  The code below copies the
        // same arguments for each thread
        args[i].x0 = x0;
        args[i].y0 = y0;
        args[i].x1 = x1;
        args[i].y1 = y1;
        args[i].width = width;
        args[i].height = height;
        args[i].maxIterations = maxIterations;
        args[i].numThreads = numThreads;
        args[i].output = output;

        args[i].threadId = i;
    }

    // Spawn the worker threads.  Note that only numThreads-1 std::threads
    // are created and the main application thread is used as a worker
    // as well.
    for (int i = 1; i < numThreads; i++) {
        workers[i] = std::thread(workerThreadStart, &args[i]);
    }

    workerThreadStart(&args[0]);

    // join worker threads
    for (int i = 1; i < numThreads; i++) {
        workers[i].join();
    }
}
