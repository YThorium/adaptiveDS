#include "adaptive_ds/AdaptiveDS.hpp"

#include <iostream>
#include <vector>
#include <list>
#include <unordered_set>
#include <set>
#include <chrono>
#include <random>
#include <fstream>
#include <iomanip>
#include <algorithm>

using namespace adaptive_ds;

struct BenchmarkMetric {
    std::string test_name;
    std::string container_name;
    size_t operation_count;
    double duration_ms;
    double ops_per_sec;
    size_t memory_bytes;
};

// High resolution timer
class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}
    void reset() { start_ = std::chrono::high_resolution_clock::now(); }
    double elapsed_ms() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

// Volatile sink to ensure compiler does not elide operations
volatile size_t g_bench_sink = 0;

// Benchmark 1: Point Lookup Heavy (90% lookups, 5% inserts, 5% deletes)
void run_lookup_heavy_benchmark(std::vector<BenchmarkMetric>& metrics) {
    std::cout << "\n>>> Running Benchmark 1: Point Lookup Heavy Workload..." << std::endl;
    const size_t N = 10000;
    const size_t NUM_OPS = 100000;

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist_elem(1, 50000);
    std::uniform_int_distribution<int> dist_op(1, 100);

    std::vector<int> initial_data;
    initial_data.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        initial_data.push_back(dist_elem(rng));
    }

    size_t local_sink = 0;

    // 1. std::unordered_set
    {
        std::unordered_set<int> uset(initial_data.begin(), initial_data.end());
        Timer timer;
        for (size_t i = 0; i < NUM_OPS; ++i) {
            int op = dist_op(rng);
            int val = dist_elem(rng);
            if (op <= 90) {
                if (uset.find(val) != uset.end()) ++local_sink;
            } else if (op <= 95) {
                uset.insert(val);
            } else {
                uset.erase(val);
            }
        }
        double ms = timer.elapsed_ms();
        size_t mem = sizeof(uset) + uset.size() * (sizeof(int) + 2 * sizeof(void*)) + uset.bucket_count() * sizeof(void*);
        metrics.push_back({"Point_Lookup_Heavy", "std::unordered_set", NUM_OPS, ms, (NUM_OPS / ms) * 1000.0, mem});
        std::cout << "  std::unordered_set: " << std::fixed << std::setprecision(2) << ms << " ms (" 
                  << (NUM_OPS / ms) * 1000.0 << " ops/sec)" << std::endl;
    }

    // 2. std::set
    {
        std::set<int> s(initial_data.begin(), initial_data.end());
        Timer timer;
        for (size_t i = 0; i < NUM_OPS; ++i) {
            int op = dist_op(rng);
            int val = dist_elem(rng);
            if (op <= 90) {
                if (s.find(val) != s.end()) ++local_sink;
            } else if (op <= 95) {
                s.insert(val);
            } else {
                s.erase(val);
            }
        }
        double ms = timer.elapsed_ms();
        size_t mem = sizeof(s) + s.size() * (sizeof(int) + 3 * sizeof(void*) + sizeof(int));
        metrics.push_back({"Point_Lookup_Heavy", "std::set", NUM_OPS, ms, (NUM_OPS / ms) * 1000.0, mem});
        std::cout << "  std::set:           " << std::fixed << std::setprecision(2) << ms << " ms (" 
                  << (NUM_OPS / ms) * 1000.0 << " ops/sec)" << std::endl;
    }

    // 3. AdaptiveDS (Starts at DynamicArray, auto-adapts to HashTable)
    {
        AdaptiveDS<int> ads(StructureType::DynamicArray);
        for (int v : initial_data) ads.insert(v);

        Timer timer;
        for (size_t i = 0; i < NUM_OPS; ++i) {
            int op = dist_op(rng);
            int val = dist_elem(rng);
            if (op <= 90) {
                if (ads.contains(val)) ++local_sink;
            } else if (op <= 95) {
                ads.insert(val);
            } else {
                ads.erase(val);
            }
        }
        double ms = timer.elapsed_ms();
        metrics.push_back({"Point_Lookup_Heavy", "AdaptiveDS (Auto)", NUM_OPS, ms, (NUM_OPS / ms) * 1000.0, ads.memory_bytes()});
        std::cout << "  AdaptiveDS (Auto):  " << std::fixed << std::setprecision(2) << ms << " ms (" 
                  << (NUM_OPS / ms) * 1000.0 << " ops/sec) -> Final: " 
                  << ads.current_structure_name() << std::endl;
    }

    g_bench_sink += local_sink;
}

// Benchmark 2: Range Query Heavy (70% range queries, 25% lookups, 5% updates)
void run_range_heavy_benchmark(std::vector<BenchmarkMetric>& metrics) {
    std::cout << "\n>>> Running Benchmark 2: Range Query Heavy Workload..." << std::endl;
    const size_t N = 5000;
    const size_t NUM_OPS = 25000;

    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> dist_elem(1, 20000);
    std::uniform_int_distribution<int> dist_op(1, 100);

    std::vector<int> initial_data;
    initial_data.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        initial_data.push_back(dist_elem(rng));
    }

    size_t local_sink = 0;

    // 1. std::set (ordered tree, efficient range)
    {
        std::set<int> s(initial_data.begin(), initial_data.end());
        Timer timer;
        for (size_t i = 0; i < NUM_OPS; ++i) {
            int op = dist_op(rng);
            int v1 = dist_elem(rng);
            if (op <= 70) {
                int low = v1;
                int high = v1 + 500;
                std::vector<int> range_res;
                auto it_low = s.lower_bound(low);
                auto it_high = s.upper_bound(high);
                for (auto it = it_low; it != it_high; ++it) {
                    range_res.push_back(*it);
                }
                local_sink += range_res.size();
            } else if (op <= 95) {
                if (s.find(v1) != s.end()) ++local_sink;
            } else {
                s.insert(v1);
            }
        }
        double ms = timer.elapsed_ms();
        size_t mem = sizeof(s) + s.size() * (sizeof(int) + 3 * sizeof(void*) + sizeof(int));
        metrics.push_back({"Range_Query_Heavy", "std::set", NUM_OPS, ms, (NUM_OPS / ms) * 1000.0, mem});
        std::cout << "  std::set:           " << std::fixed << std::setprecision(2) << ms << " ms (" 
                  << (NUM_OPS / ms) * 1000.0 << " ops/sec)" << std::endl;
    }

    // 2. std::unordered_set (must scan all elements and sort for range queries)
    {
        std::unordered_set<int> uset(initial_data.begin(), initial_data.end());
        Timer timer;
        for (size_t i = 0; i < NUM_OPS; ++i) {
            int op = dist_op(rng);
            int v1 = dist_elem(rng);
            if (op <= 70) {
                int low = v1;
                int high = v1 + 500;
                std::vector<int> range_res;
                for (int x : uset) {
                    if (x >= low && x <= high) range_res.push_back(x);
                }
                std::sort(range_res.begin(), range_res.end());
                local_sink += range_res.size();
            } else if (op <= 95) {
                if (uset.find(v1) != uset.end()) ++local_sink;
            } else {
                uset.insert(v1);
            }
        }
        double ms = timer.elapsed_ms();
        size_t mem = sizeof(uset) + uset.size() * (sizeof(int) + 2 * sizeof(void*)) + uset.bucket_count() * sizeof(void*);
        metrics.push_back({"Range_Query_Heavy", "std::unordered_set", NUM_OPS, ms, (NUM_OPS / ms) * 1000.0, mem});
        std::cout << "  std::unordered_set: " << std::fixed << std::setprecision(2) << ms << " ms (" 
                  << (NUM_OPS / ms) * 1000.0 << " ops/sec)" << std::endl;
    }

    // 3. AdaptiveDS (Starts at HashTable, adapts to AVLTree for range speedup)
    {
        AdaptiveDS<int> ads(StructureType::HashTable);
        for (int v : initial_data) ads.insert(v);

        Timer timer;
        for (size_t i = 0; i < NUM_OPS; ++i) {
            int op = dist_op(rng);
            int v1 = dist_elem(rng);
            if (op <= 70) {
                int low = v1;
                int high = v1 + 500;
                auto res = ads.range(low, high);
                local_sink += res.size();
            } else if (op <= 95) {
                if (ads.contains(v1)) ++local_sink;
            } else {
                ads.insert(v1);
            }
        }
        double ms = timer.elapsed_ms();
        metrics.push_back({"Range_Query_Heavy", "AdaptiveDS (Auto)", NUM_OPS, ms, (NUM_OPS / ms) * 1000.0, ads.memory_bytes()});
        std::cout << "  AdaptiveDS (Auto):  " << std::fixed << std::setprecision(2) << ms << " ms (" 
                  << (NUM_OPS / ms) * 1000.0 << " ops/sec) -> Final: " 
                  << ads.current_structure_name() << std::endl;
    }

    g_bench_sink += local_sink;
}

// Benchmark 3: Dynamic Multi-Phase Shifting Workload
void run_dynamic_phase_shifting_benchmark(std::vector<BenchmarkMetric>& metrics) {
    std::cout << "\n>>> Running Benchmark 3: Multi-Phase Shifting Workload..." << std::endl;
    std::cout << "  Phase 1: Small dataset inserts (N=16, DynamicArray cache optimal)" << std::endl;
    std::cout << "  Phase 2: Point lookup storm (50,000 lookups, HashTable optimal)" << std::endl;
    std::cout << "  Phase 3: Range query storm (20,000 range scans, AVLTree optimal)" << std::endl;
    std::cout << "  Phase 4: Mixed workload stabilization" << std::endl;

    std::mt19937 rng(999);
    std::uniform_int_distribution<int> dist_val(1, 25000);
    size_t local_sink = 0;

    // 1. AdaptiveDS (Autonomous Self-Optimization)
    {
        AdaptiveDS<int> ads(StructureType::DynamicArray);
        Timer total_timer;

        // Phase 1: Small dataset insertions (N=16)
        for (int i = 0; i < 16; ++i) ads.insert(i * 10);

        // Phase 2: Scale up and run 50,000 point lookups
        for (int i = 0; i < 3000; ++i) ads.insert(dist_val(rng));
        for (int i = 0; i < 50000; ++i) {
            if (ads.contains(dist_val(rng))) ++local_sink;
        }

        // Phase 3: 20,000 range queries
        for (int i = 0; i < 20000; ++i) {
            int low = dist_val(rng);
            auto r = ads.range(low, low + 500);
            local_sink += r.size();
        }

        // Phase 4: Mixed stabilization
        for (int i = 0; i < 10000; ++i) {
            int v = dist_val(rng);
            if (i % 2 == 0) {
                if (ads.contains(v)) ++local_sink;
            } else {
                ads.insert(v);
            }
        }

        double total_ms = total_timer.elapsed_ms();
        size_t total_ops = 16 + 3000 + 50000 + 20000 + 10000;
        metrics.push_back({"Dynamic_Multi_Phase", "AdaptiveDS (Self-Optimizing)", total_ops, total_ms, (total_ops / total_ms) * 1000.0, ads.memory_bytes()});

        std::cout << "  -> AdaptiveDS Total: " << std::fixed << std::setprecision(2) << total_ms << " ms (" 
                  << (total_ops / total_ms) * 1000.0 << " ops/sec)" << std::endl;
        std::cout << "     Switches executed: " << ads.get_adaptation_history().size() << std::endl;
        for (const auto& rec : ads.get_adaptation_history()) {
            std::cout << "       [" << to_string(rec.from_structure) << " -> " 
                      << to_string(rec.to_structure) << "] at op " << rec.operation_tick 
                      << " (migration: " << rec.migration_duration_us << " us)" << std::endl;
        }

        ads.export_history_csv("adaptation_history.csv");
    }

    // 2. Static std::unordered_set over exact same multi-phase workload
    {
        std::mt19937 rng_uset(999);
        std::unordered_set<int> uset;
        Timer total_timer;

        for (int i = 0; i < 16; ++i) uset.insert(i * 10);
        for (int i = 0; i < 3000; ++i) uset.insert(dist_val(rng_uset));
        for (int i = 0; i < 50000; ++i) {
            if (uset.find(dist_val(rng_uset)) != uset.end()) ++local_sink;
        }
        for (int i = 0; i < 20000; ++i) {
            int low = dist_val(rng_uset);
            int high = low + 500;
            std::vector<int> range_res;
            for (int x : uset) {
                if (x >= low && x <= high) range_res.push_back(x);
            }
            std::sort(range_res.begin(), range_res.end());
            local_sink += range_res.size();
        }
        for (int i = 0; i < 10000; ++i) {
            int v = dist_val(rng_uset);
            if (i % 2 == 0) {
                if (uset.find(v) != uset.end()) ++local_sink;
            } else {
                uset.insert(v);
            }
        }

        double total_ms = total_timer.elapsed_ms();
        size_t total_ops = 16 + 3000 + 50000 + 20000 + 10000;
        size_t mem = sizeof(uset) + uset.size() * (sizeof(int) + 2 * sizeof(void*)) + uset.bucket_count() * sizeof(void*);
        metrics.push_back({"Dynamic_Multi_Phase", "Static std::unordered_set", total_ops, total_ms, (total_ops / total_ms) * 1000.0, mem});
        std::cout << "  -> Static std::unordered_set Total: " << std::fixed << std::setprecision(2) << total_ms 
                  << " ms (" << (total_ops / total_ms) * 1000.0 << " ops/sec)" << std::endl;
    }

    // 3. Static std::set over exact same multi-phase workload
    {
        std::mt19937 rng_set(999);
        std::set<int> s;
        Timer total_timer;

        for (int i = 0; i < 16; ++i) s.insert(i * 10);
        for (int i = 0; i < 3000; ++i) s.insert(dist_val(rng_set));
        for (int i = 0; i < 50000; ++i) {
            if (s.find(dist_val(rng_set)) != s.end()) ++local_sink;
        }
        for (int i = 0; i < 20000; ++i) {
            int low = dist_val(rng_set);
            int high = low + 500;
            std::vector<int> range_res;
            auto it_low = s.lower_bound(low);
            auto it_high = s.upper_bound(high);
            for (auto it = it_low; it != it_high; ++it) {
                range_res.push_back(*it);
            }
            local_sink += range_res.size();
        }
        for (int i = 0; i < 10000; ++i) {
            int v = dist_val(rng_set);
            if (i % 2 == 0) {
                if (s.find(v) != s.end()) ++local_sink;
            } else {
                s.insert(v);
            }
        }

        double total_ms = total_timer.elapsed_ms();
        size_t total_ops = 16 + 3000 + 50000 + 20000 + 10000;
        size_t mem = sizeof(s) + s.size() * (sizeof(int) + 3 * sizeof(void*) + sizeof(int));
        metrics.push_back({"Dynamic_Multi_Phase", "Static std::set", total_ops, total_ms, (total_ops / total_ms) * 1000.0, mem});
        std::cout << "  -> Static std::set Total: " << std::fixed << std::setprecision(2) << total_ms 
                  << " ms (" << (total_ops / total_ms) * 1000.0 << " ops/sec)" << std::endl;
    }

    g_bench_sink += local_sink;
}

void export_metrics_to_csv(const std::vector<BenchmarkMetric>& metrics, const std::string& filepath) {
    std::ofstream ofs(filepath);
    if (!ofs.is_open()) {
        std::cerr << "Failed to open CSV output: " << filepath << std::endl;
        return;
    }

    ofs << "test_name,container_name,operation_count,duration_ms,ops_per_sec,memory_bytes\n";
    for (const auto& m : metrics) {
        ofs << m.test_name << ","
            << "\"" << m.container_name << "\","
            << m.operation_count << ","
            << std::fixed << std::setprecision(3) << m.duration_ms << ","
            << std::setprecision(2) << m.ops_per_sec << ","
            << m.memory_bytes << "\n";
    }
    std::cout << "\nExported benchmark results to: " << filepath << std::endl;
}

int main() {
    std::cout << "=====================================================" << std::endl;
    std::cout << " AdaptiveDS vs STL Containers Benchmark Suite" << std::endl;
    std::cout << "=====================================================" << std::endl;

    std::vector<BenchmarkMetric> metrics;
    run_lookup_heavy_benchmark(metrics);
    run_range_heavy_benchmark(metrics);
    run_dynamic_phase_shifting_benchmark(metrics);

    export_metrics_to_csv(metrics, "benchmark_results.csv");

    std::cout << "\nAll benchmarks completed successfully." << std::endl;
    return 0;
}
