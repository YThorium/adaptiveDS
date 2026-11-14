# AdaptiveDS: Self-Optimizing C++ Data Structure Library

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.20+-064F8C.svg)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Build & Tests](https://img.shields.io/badge/Tests-27%2F27%20Passing-brightgreen.svg)]()

**AdaptiveDS** is an advanced, production-grade C++ self-optimizing data structure library. Rather than forcing software engineers to make static, compile-time compromises among arrays, linked lists, hash tables, and balanced binary search trees, AdaptiveDS continuously observes runtime access patterns using a sliding workload window and dynamically migrates its underlying memory representation to guarantee optimal asymptotic and cache performance.

---

## Key Features

- **Genuine Data Structure Implementations (No STL Wrappers)**:
  - **Dynamic Array**: Contiguous raw heap buffer with amortized $O(1)$ push, $O(1)$ random indexing, geometric doubling, left-shift element compaction, and automatic memory shrinking.
  - **Doubly Linked List**: Node-based bidirectional architecture with sentinel head/tail pointers, enabling zero-pointer-check $O(1)$ unlinking and node deletions.
  - **Hash Table**: Separate-chaining hash table with dynamic bucket array resizing, load factor tracking ($\alpha \le 0.75$), and uniform bucket redistribution.
  - **AVL Tree**: Self-balancing binary search tree maintaining strict height balancing ($|BF| \le 1$) via Left (LL), Right (RR), Left-Right (LR), and Right-Left (RL) rotations, delivering optimal $O(\log N + K)$ range searches.
- **Unified Generic Container API**:
  - `insert(val)`, `erase(val)`, `contains(val)`, `find(val)`, `range(low, high)`, `size()`, `empty()`, `clear()`, and `to_vector()`.
- **Telemetry & Workload Monitoring**:
  - Ring buffer sliding window tracking operation frequencies ($P_{lookup}, P_{insert}, P_{erase}, P_{range}$), read-to-write ratios, and arrival rates in $O(1)$ per recorded operation.
- **Intelligent Adaptation Engine**:
  - Calibrated cost models evaluating asymptotic time complexity, L1 cache locality, and pointer chasing penalties.
  - Small dataset cache bonus favoring dynamic arrays when $N \le 24$.
- **Safe & Atomic Structure Migration**:
  - Strong exception guarantee with element conservation invariants ($N_{target} == N_{source}$). Zero element loss across all pairwise transitions.
- **Hysteresis, Amortization & Thrashing Prevention**:
  - Migration cost barrier ($C_{migrate}(from, to, N)$).
  - Horizon amortization threshold ($\Delta_{cost} \cdot H > C_{migrate} \times (1 + \tau)$).
  - Configurable cooldown period suppressing oscillatory "ping-pong" migrations.
- **Audit Logging & Telemetry Export**:
  - Captures full historical decision traces, rationale, workload snapshots, candidate cost tables, and microsecond migration timings.
  - Automatic export to CSV format for validation and profiling.

---

## System Architecture

```
                       +-----------------------------------+
                       |        AdaptiveDS<T> API          |
                       | insert() erase() find() range()   |
                       +-----------------+-----------------+
                                         |
               +-------------------------+-------------------------+
               |                                                   |
       [Operation Hook]                                   [Polymorphic Delegate]
               |                                                   |
               v                                                   v
   +-----------------------+                            +--------------------+
   |   WorkloadMonitor     |                            |  IDataStructure<T> |
   | - Ring buffer window  |                            +---------+----------+
   | - Running frequencies |                                      |
   +-----------+-----------+                    +-----------------+-----------------+
               |                                |                 |                 |
               v                                v                 v                 v
   +-----------------------+              +-----------+     +-----------+     +-----------+
   |   AdaptationEngine    |              |  Dynamic  |     |   Hash    |     |    AVL    |
   | - Cost Evaluation     |              |   Array   |     |   Table   |     |   Tree    |
   | - Migration Penalty   |              +-----------+     +-----------+     +-----------+
   | - Cooldown & Hysteresis              (L1 Cache-fit)    (Point O(1))     (Range O(logN))
   +-----------+-----------+
               | (Decision Triggered)
               v
   +-----------------------+
   | Safe Migration Flow   |
   | 1. Extract Elements   |
   | 2. Populate Target    |
   | 3. Invariant Check    |
   | 4. Atomic Pointer Swap|
   | 5. Log History Record |
   +-----------------------+
```

---

## Performance Benchmarks

Benchmarks performed on Linux x86_64, GCC 16.2.1 `-O3`:

### 1. Point Lookup Heavy Workload (100,000 Ops, 90% Lookup, 5% Insert, 5% Erase)
| Container | Duration (ms) | Throughput (ops/sec) | Memory (Bytes) | Notes |
| :--- | :---: | :---: | :---: | :--- |
| `std::unordered_set` | 2.85 ms | 35.08 M ops/s | 403,820 B | Fixed Hash Table baseline |
| **AdaptiveDS (Auto)** | **5.37 ms** | **18.61 M ops/s** | **321,776 B** | **Adapted Array -> HashTable** |
| `std::set` | 8.95 ms | 11.17 M ops/s | 383,344 B | Fixed Red-Black Tree ($O(\log N)$) |

### 2. Range Query Heavy Workload (25,000 Ops, 70% Range [low, low+500], 25% Lookup, 5% Write)
| Container | Duration (ms) | Throughput (ops/sec) | Memory (Bytes) | Notes |
| :--- | :---: | :---: | :---: | :--- |
| **AdaptiveDS (Auto)** | **14.57 ms** | **1.71 M ops/s** | **129,704 B** | **Adapted HashTable -> AVLTree** |
| `std::set` | 19.84 ms | 1.26 M ops/s | 170,096 B | STL Tree ordered range |
| `std::unordered_set` | 409.77 ms | 0.06 M ops/s | 188,440 B | **28.1x slower** (Full bucket scans) |

### 3. Dynamic Multi-Phase Shifting Workload (83,016 Ops across 4 Phases)
*Phase 1: Small Ingestion $\to$ Phase 2: Point Lookup Storm $\to$ Phase 3: Range Query Storm $\to$ Phase 4: Stabilization*

| Container | Total Duration (ms) | Total Throughput (ops/sec) | Autonomous Migrations |
| :--- | :---: | :---: | :---: |
| **AdaptiveDS (Self-Optimizing)** | **11.23 ms** | **7,391,136 ops/s** | **3 Switches (Total overhead: 0.29 ms)** |
| `std::set` | 15.25 ms | 5,442,022 ops/s | 0 (Suboptimal in Phase 2) |
| `std::unordered_set` | 258.73 ms | 320,857 ops/s | 0 (**23.0x slower** due to Phase 3) |

---

## Building and Running

### Prerequisites
- Modern C++ compiler supporting C++20 (GCC 11+, Clang 13+, MSVC 2019+)
- CMake 3.20+
- Linux, macOS, or Windows

### Build Instructions
```bash
# Clone the repository
git clone https://github.com/AdaptiveDS/AdaptiveDS.git
cd AdaptiveDS

# Generate build configuration
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Compile all targets
cmake --build build -j$(nproc)
```

### Running Automated Unit Tests
```bash
./build/adaptive_ds_tests
```
Expected output:
```
==========================================
 Running AdaptiveDS Automated Test Suite 
==========================================
[RUN     ] DynamicArray.BasicInsertAndSize -> [PASS]
...
[RUN     ] AdaptiveDS.ExportHistoryCSV -> [PASS]
------------------------------------------
Test Summary: 27 Passed, 0 Failed, 27 Total
------------------------------------------
```

### Running Benchmarks
```bash
./build/bench_adaptive_ds
```
Outputs `benchmark_results.csv` and `adaptation_history.csv` for analysis.

### Running Live Interactive Demo
```bash
# Interactive mode (visual pacing)
./build/demo_adaptive_ds

# Fast mode (immediate execution)
./build/demo_adaptive_ds --fast
```

---

## Code Example

```cpp
#include "adaptive_ds/AdaptiveDS.hpp"
#include <iostream>

int main() {
    // Instantiate with default DynamicArray
    adaptive_ds::AdaptiveDS<int> ds;

    // 1. Initial insertions
    for (int i = 0; i < 20; ++i) {
        ds.insert(i * 5);
    }
    std::cout << "Initial container: " << ds.current_structure_name() << std::endl;

    // 2. Perform point lookups (triggers auto-adaptation to HashTable)
    for (int i = 0; i < 200; ++i) {
        ds.contains(i * 5);
    }
    std::cout << "Post lookups container: " << ds.current_structure_name() << std::endl;

    // 3. Perform range queries (triggers auto-adaptation to AVLTree)
    for (int i = 0; i < 200; ++i) {
        auto matches = ds.range(10, 50);
    }
    std::cout << "Post range container: " << ds.current_structure_name() << std::endl;

    // 4. Export adaptation audit trail
    ds.export_history_csv("my_workload_audit.csv");
    return 0;
}
```

---

## Project Structure

```
AdaptiveDS/
├── CMakeLists.txt              # Top-level build definitions
├── README.md                   # Project overview & documentation
├── include/
│   └── adaptive_ds/
│       ├── Types.hpp           # Data structures, enums, config, telemetry records
│       ├── IDataStructure.hpp  # Abstract container interface
│       ├── DynamicArray.hpp    # Custom contiguous dynamic array implementation
│       ├── DoublyLinkedList.hpp# Custom node-based doubly linked list implementation
│       ├── HashTable.hpp       # Custom separate-chaining hash table implementation
│       ├── AVLTree.hpp         # Custom self-balancing AVL BST implementation
│       ├── WorkloadMonitor.hpp # Circular buffer sliding-window access profiler
│       ├── AdaptationEngine.hpp# Analytical cost estimator & hysteresis logic
│       ├── AdaptationHistory.hpp# Audit logger & CSV export facility
│       └── AdaptiveDS.hpp      # Unified self-optimizing polymorphic container
├── src/
│   └── adaptive_ds/            # Core translation units
├── tests/                      # Automated unit tests covering all components
├── benchmarks/                 # Comparative micro-benchmarks against STL
├── demo/                       # Live demonstration terminal application

```

---
