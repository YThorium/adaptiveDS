#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace adaptive_ds {

enum class StructureType {
    DynamicArray,
    DoublyLinkedList,
    HashTable,
    AVLTree
};

inline const char* to_string(StructureType type) {
    switch (type) {
        case StructureType::DynamicArray:    return "DynamicArray";
        case StructureType::DoublyLinkedList: return "DoublyLinkedList";
        case StructureType::HashTable:       return "HashTable";
        case StructureType::AVLTree:         return "AVLTree";
    }
    return "Unknown";
}

enum class OperationType {
    Insert,
    Erase,
    Find,
    Contains,
    RangeQuery,
    Iterate
};

inline const char* to_string(OperationType op) {
    switch (op) {
        case OperationType::Insert:     return "Insert";
        case OperationType::Erase:      return "Erase";
        case OperationType::Find:       return "Find";
        case OperationType::Contains:   return "Contains";
        case OperationType::RangeQuery: return "RangeQuery";
        case OperationType::Iterate:    return "Iterate";
    }
    return "Unknown";
}

struct OperationRecord {
    OperationType type;
    uint64_t timestamp_us{0};
    size_t result_count{0};
};

struct WorkloadStats {
    size_t total_operations{0};
    size_t insert_count{0};
    size_t erase_count{0};
    size_t find_count{0};
    size_t contains_count{0};
    size_t range_count{0};
    size_t iterate_count{0};

    // Frequencies (0.0 to 1.0)
    double insert_freq{0.0};
    double erase_freq{0.0};
    double lookup_freq{0.0}; // find + contains
    double range_freq{0.0};
    double iterate_freq{0.0};

    // High level metrics
    double read_write_ratio{0.0}; // (reads)/(writes + 1)
    size_t window_size{0};
};

struct AdaptationConfig {
    size_t sliding_window_size{500};
    size_t min_operations_before_adaptation{100};
    size_t cooldown_operations{250};
    double improvement_threshold{0.25}; // Needs 25% cost reduction to trigger switch
    size_t small_dataset_threshold{24};  // Below this, cache locality makes array preferred
    size_t projection_horizon{1000};    // Projected future operations to amortize migration cost
    size_t check_frequency{16};         // Periodically sample adaptation every N operations
    bool enable_automatic_adaptation{true};
    bool verbose_logging{false};
};

struct StructureCostEstimate {
    StructureType type;
    double lookup_cost{0.0};
    double insert_cost{0.0};
    double erase_cost{0.0};
    double range_cost{0.0};
    double total_estimated_cost{0.0};
};

struct AdaptationRecord {
    uint64_t id{0};
    uint64_t timestamp_us{0};
    size_t operation_tick{0};
    StructureType from_structure{StructureType::DynamicArray};
    StructureType to_structure{StructureType::DynamicArray};
    size_t element_count{0};
    double migration_duration_us{0.0};
    std::string reason;
    WorkloadStats workload_snapshot;
    std::vector<StructureCostEstimate> candidate_costs;
};

} // namespace adaptive_ds
