#include "adaptive_ds/AdaptiveDS.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#include <random>

using namespace adaptive_ds;

void print_banner() {
    std::cout << "\033[1;36m";
    std::cout << "====================================================================\n";
    std::cout << "          AdaptiveDS - Self-Optimizing C++ Data Structure            \n";
    std::cout << "                   Live Demonstration Application                    \n";
    std::cout << "====================================================================\n";
    std::cout << "\033[0m";
}

void print_status_box(const AdaptiveDS<int>& ds, const std::string& phase_title) {
    WorkloadStats stats = ds.get_stats();
    std::cout << "\n\033[1;33m--------------------------------------------------------------------\033[0m\n";
    std::cout << "\033[1;32m[Active Phase]\033[0m: " << phase_title << "\n";
    std::cout << "\033[1;34m[Current Structure]\033[0m: \033[1;35m" << ds.current_structure_name() << "\033[0m"
              << " | \033[1;34m[Elements]\033[0m: " << ds.size()
              << " | \033[1;34m[Memory]\033[0m: " << (ds.memory_bytes() / 1024.0) << " KB\n";

    std::cout << "Sliding Window (" << stats.window_size << " ops): "
              << "Lookup: " << std::fixed << std::setprecision(1) << (stats.lookup_freq * 100.0) << "% | "
              << "Range: " << (stats.range_freq * 100.0) << "% | "
              << "Insert: " << (stats.insert_freq * 100.0) << "% | "
              << "Erase: " << (stats.erase_freq * 100.0) << "%\n";

    // Progress bar for lookups vs ranges
    int lookup_bars = static_cast<int>(stats.lookup_freq * 20.0);
    int range_bars = static_cast<int>(stats.range_freq * 20.0);
    int other_bars = 20 - lookup_bars - range_bars;
    if (other_bars < 0) other_bars = 0;

    std::cout << "Workload Profile: [";
    std::cout << "\033[32m" << std::string(lookup_bars, '#') << "\033[0m";
    std::cout << "\033[36m" << std::string(range_bars, '=') << "\033[0m";
    std::cout << "\033[37m" << std::string(other_bars, '.') << "\033[0m";
    std::cout << "] (Legend: # Lookup, = Range, . Write)\n";
    std::cout << "\033[1;33m--------------------------------------------------------------------\033[0m\n";
}

int main(int argc, char* argv[]) {
    bool fast_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--fast") {
            fast_mode = true;
        }
    }

    auto sleep_ms = [fast_mode](int ms) {
        if (!fast_mode) {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        }
    };

    print_banner();

    AdaptationConfig cfg;
    cfg.sliding_window_size = 100;
    cfg.min_operations_before_adaptation = 50;
    cfg.cooldown_operations = 60;
    cfg.improvement_threshold = 0.20;
    cfg.projection_horizon = 500;
    cfg.small_dataset_threshold = 24;
    cfg.enable_automatic_adaptation = true;
    cfg.verbose_logging = true;

    // Initialize with DynamicArray
    AdaptiveDS<int> ads(StructureType::DynamicArray, cfg);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist_key(1, 10000);

    // Scenario 1: Small Dataset
    std::cout << "\n\033[1;32m>>> Scenario 1: Initial Ingestion & Small Dataset (N=16)\033[0m\n";
    std::cout << "Inserting 16 items into initial container...\n";
    for (int i = 1; i <= 16; ++i) {
        ads.insert(i * 10);
    }
    print_status_box(ads, "Small Dataset Ingestion (L1 Cache Optimized)");
    sleep_ms(400);

    // Scenario 2: Scale and Point Lookup Storm
    std::cout << "\n\033[1;32m>>> Scenario 2: Scale up to 1,500 elements & Trigger Point Lookup Storm\033[0m\n";
    for (int i = 1; i <= 1500; ++i) {
        ads.insert(dist_key(rng));
    }
    std::cout << "Executing 400 point lookups (find/contains) in tight loop...\n";
    for (int i = 0; i < 400; ++i) {
        ads.contains(dist_key(rng));
        if (i == 120 || i == 300) {
            print_status_box(ads, "Point Lookup Storm in progress");
            sleep_ms(200);
        }
    }
    print_status_box(ads, "Post Point-Lookup Storm");
    sleep_ms(400);

    // Scenario 3: Workload Phase Shift to Range Queries
    std::cout << "\n\033[1;32m>>> Scenario 3: Workload Phase Shift to Range Queries\033[0m\n";
    std::cout << "Executing 350 range queries [key, key + 500]...\n";
    for (int i = 0; i < 350; ++i) {
        int low = dist_key(rng);
        auto res = ads.range(low, low + 500);
        (void)res;
        if (i == 100 || i == 250) {
            print_status_box(ads, "Range Query Heavy Phase in progress");
            sleep_ms(200);
        }
    }
    print_status_box(ads, "Post Range-Query Storm");
    sleep_ms(400);

    // Scenario 4: Review Adaptation History
    std::cout << "\n\033[1;32m>>> Scenario 4: Self-Optimization Audit Log\033[0m\n";
    const auto& history = ads.get_adaptation_history();
    std::cout << "Recorded " << history.size() << " automated structure migrations:\n";
    for (size_t i = 0; i < history.size(); ++i) {
        const auto& rec = history[i];
        std::cout << "\n  [\033[1;33mEvent #" << (i + 1) << "\033[0m] Op Tick " << rec.operation_tick << ":\n";
        std::cout << "    Transition: " << to_string(rec.from_structure) << " -> " 
                  << to_string(rec.to_structure) << "\n";
        std::cout << "    Dataset Size: " << rec.element_count << " elements\n";
        std::cout << "    Migration Duration: " << rec.migration_duration_us << " microseconds\n";
        std::cout << "    Decision Rationale: " << rec.reason << "\n";
    }

    std::string csv_file = "demo_adaptation_history.csv";
    if (ads.export_history_csv(csv_file)) {
        std::cout << "\n\033[1;32mSuccessfully saved adaptation history to: " << csv_file << "\033[0m\n";
    }

    std::cout << "\n\033[1;36m====================================================================\n";
    std::cout << "             Demonstration Completed Successfully!                  \n";
    std::cout << "====================================================================\033[0m\n";

    return 0;
}
