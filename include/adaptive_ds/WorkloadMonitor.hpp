#pragma once

#include "Types.hpp"
#include <vector>
#include <cstddef>
#include <chrono>

namespace adaptive_ds {

class WorkloadMonitor {
public:
    explicit WorkloadMonitor(size_t window_capacity = 500);

    void record_operation(OperationType op, size_t result_count = 1);

    WorkloadStats get_stats() const;

    void reset();

    size_t window_capacity() const noexcept { return window_capacity_; }
    size_t current_window_size() const noexcept { return current_size_; }
    size_t total_operations() const noexcept { return lifetime_operations_; }

    void resize_window(size_t new_capacity);

private:
    uint64_t current_time_micros() const;

    size_t window_capacity_;
    size_t head_index_{0};
    size_t current_size_{0};
    size_t lifetime_operations_{0};

    std::vector<OperationRecord> window_buffer_;

    // Running window counts
    size_t window_insert_count_{0};
    size_t window_erase_count_{0};
    size_t window_find_count_{0};
    size_t window_contains_count_{0};
    size_t window_range_count_{0};
    size_t window_iterate_count_{0};

    // Lifetime counts
    size_t lifetime_insert_count_{0};
    size_t lifetime_erase_count_{0};
    size_t lifetime_find_count_{0};
    size_t lifetime_contains_count_{0};
    size_t lifetime_range_count_{0};
    size_t lifetime_iterate_count_{0};
};

} // namespace adaptive_ds
