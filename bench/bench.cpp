#include "allocator.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

using namespace std;
using Clock = chrono::high_resolution_clock;

inline void touch(void* p) {
    *(volatile char*)p = 1;
}

// Median runtime over multiple trials
template <typename Func>
double median_time_ms(Func f, int trials = 5) {
    vector<double> times;

    for (int i = 0; i < trials; i++) {
        auto start = Clock::now();
        f();
        auto end = Clock::now();

        times.push_back(chrono::duration<double, milli>(end - start).count());
    }

    sort(times.begin(), times.end());
    return times[times.size() / 2];
}

void report(const string& label, double ms) {
    cout << left << setw(40) << label
         << right << setw(10)
         << fixed << setprecision(3)
         << ms << " ms\n";
}

void scenario_single_threaded() {
    const int N = 1000000;

    cout << "\n=== Scenario 1: Single-threaded, fixed 32B size, N=" << N << " ===\n";

    double t_glibc = median_time_ms([&]() {
        for (int i = 0; i < N; i++) {
            void* p = malloc(32);
            touch(p);
            free(p);
        }
    });

    double t_mine = median_time_ms([&]() {
        for (int i = 0; i < N; i++) {
            void* p = ma::alloc(32);
            touch(p);
            ma::free(p, 32);
        }
    });

    report("glibc malloc/free", t_glibc);
    report("ma::alloc/free", t_mine);

    cout << "Result: "
         << (t_glibc < t_mine ? "glibc wins" : "ma:: wins")
         << " (" << setprecision(2) << (t_glibc / t_mine) << "x)\n";
}

void scenario_varied_sizes() {
    const int N = 1000000;

    cout << "\n=== Scenario 2: Single-threaded, varied sizes 8-512B, N=" << N << " ===\n";

    mt19937 rng(42);
    uniform_int_distribution<int> dist(8, 512);

    vector<size_t> sizes(N);
    for (int i = 0; i < N; i++)
        sizes[i] = dist(rng);

    double t_glibc = median_time_ms([&]() {
        for (int i = 0; i < N; i++) {
            void* p = malloc(sizes[i]);
            touch(p);
            free(p);
        }
    });

    double t_mine = median_time_ms([&]() {
        for (int i = 0; i < N; i++) {
            void* p = ma::alloc(sizes[i]);
            touch(p);
            ma::free(p, sizes[i]);
        }
    });

    report("glibc malloc/free", t_glibc);
    report("ma::alloc/free", t_mine);

    cout << "Result: "
         << (t_glibc < t_mine ? "glibc wins" : "ma:: wins")
         << " (" << setprecision(2) << (t_glibc / t_mine) << "x)\n";
}

// Worker for glibc
void worker_glibc(int iterations) {
    void* warm = malloc(32);
    free(warm);

    for (int i = 0; i < iterations; i++) {
        void* p = malloc(32);
        touch(p);
        free(p);
    }
}

// Worker for Mnemosyne
void worker_mine(int iterations) {
    void* warm = ma::alloc(32);
    ma::free(warm, 32);

    for (int i = 0; i < iterations; i++) {
        void* p = ma::alloc(32);
        touch(p);
        ma::free(p, 32);
    }
}

void scenario_multithreaded_scaling() {
    cout << "\n=== Scenario 3: Multithreaded scaling, fixed 32B size ===\n";

    cout << left << setw(10) << "Threads"
         << setw(16) << "glibc (ms)"
         << setw(16) << "ma:: (ms)"
         << "Result\n";

    int thread_counts[] = {1, 2, 4, 8, 16};
    const int ITER = 200000;

    for (int n : thread_counts) {

        double t_glibc = median_time_ms([&]() {
            vector<thread> threads;

            for (int i = 0; i < n; i++)
                threads.emplace_back(worker_glibc, ITER);

            for (auto& t : threads)
                t.join();
        }, 3);

        double t_mine = median_time_ms([&]() {
            vector<thread> threads;

            for (int i = 0; i < n; i++)
                threads.emplace_back(worker_mine, ITER);

            for (auto& t : threads)
                t.join();
        }, 3);

        string winner = (t_glibc < t_mine) ? "glibc" : "ma::";
        double ratio = (t_glibc < t_mine)
                           ? (t_mine / t_glibc)
                           : (t_glibc / t_mine);

        cout << left << setw(10) << n
             << setw(16) << fixed << setprecision(2) << t_glibc
             << setw(16) << t_mine
             << winner << " by " << ratio << "x\n";
    }
}

void scenario_large_allocations() {
    const int N = 200000;

    cout << "\n=== Scenario 4: Large allocations (4KB, bypasses slab path) ===\n";

    double t_glibc = median_time_ms([&]() {
        for (int i = 0; i < N; i++) {
            void* p = malloc(4096);
            touch(p);
            free(p);
        }
    });

    double t_mine = median_time_ms([&]() {
        for (int i = 0; i < N; i++) {
            void* p = ma::alloc(4096);
            touch(p);
            ma::free(p, 4096);
        }
    });

    report("glibc malloc/free", t_glibc);
    report("ma::alloc/free", t_mine);

    cout << "Result: "
         << (t_glibc < t_mine ? "glibc wins" : "ma:: wins")
         << " (" << setprecision(2) << (t_glibc / t_mine) << "x)\n";

    cout << "(Expected: glibc wins since large allocations bypass the slab allocator.)\n";
}

void scenario_allocate_heavy() {
    const int N = 500000;

    cout << "\n=== Scenario 5: Allocate " << N
         << " objects, hold, then free all ===\n";

    double t_glibc = median_time_ms([&]() {
        vector<void*> ptrs;
        ptrs.reserve(N);

        for (int i = 0; i < N; i++) {
            void* p = malloc(64);
            touch(p);
            ptrs.push_back(p);
        }

        for (void* p : ptrs)
            free(p);
    }, 3);

    double t_mine = median_time_ms([&]() {
        vector<void*> ptrs;
        ptrs.reserve(N);

        for (int i = 0; i < N; i++) {
            void* p = ma::alloc(64);
            touch(p);
            ptrs.push_back(p);
        }

        for (void* p : ptrs)
            ma::free(p, 64);
    }, 3);

    report("glibc malloc/free", t_glibc);
    report("ma::alloc/free", t_mine);

    cout << "Result: "
         << (t_glibc < t_mine ? "glibc wins" : "ma:: wins")
         << " (" << setprecision(2) << (t_glibc / t_mine) << "x)\n";
}

int main() {
    ma::init();

    cout << "Each result is the MEDIAN of multiple trials (noise-resistant).\n";

    scenario_single_threaded();
    scenario_varied_sizes();
    scenario_multithreaded_scaling();
    scenario_large_allocations();
    scenario_allocate_heavy();

    cout << '\n';

    ma::shutdown();
    return 0;
}