#include "adaptive_ds/WorkloadMonitor.hpp"

namespace adaptive_ds {

WorkloadMonitor::WorkloadMonitor(size_t window_capacity)
    : window_capacity_(window_capacity > 10 ? window_capacity : 10),
      head_index_(0),
      current_size_(0),
      lifetime_operations_(0),
      window_buffer_(window_capacity_) {}

uint64_t WorkloadMonitor::current_time_micros() const {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
}

void WorkloadMonitor::record_operation(OperationType op, size_t result_count) {
    ++lifetime_operations_;
    switch (op) {
        case OperationType::Insert:     ++lifetime_insert_count_; break;
        case OperationType::Erase:      ++lifetime_erase_count_; break;
        case OperationType::Find:       ++lifetime_find_count_; break;
        case OperationType::Contains:   ++lifetime_contains_count_; break;
        case OperationType::RangeQuery: ++lifetime_range_count_; break;
        case OperationType::Iterate:    ++lifetime_iterate_count_; break;
    }

    // If window is full, remove the evicted operation from counts
    if (current_size_ == window_capacity_) {
        const OperationRecord& old_rec = window_buffer_[head_index_];
        switch (old_rec.type) {
            case OperationType::Insert:     --window_insert_count_; break;
            case OperationType::Erase:      --window_erase_count_; break;
            case OperationType::Find:       --window_find_count_; break;
            case OperationType::Contains:   --window_contains_count_; break;
            case OperationType::RangeQuery: --window_range_count_; break;
            case OperationType::Iterate:    --window_iterate_count_; break;
        }
    } else {
        ++current_size_;
    }

    // Add new operation
    switch (op) {
        case OperationType::Insert:     ++window_insert_count_; break;
        case OperationType::Erase:      ++window_erase_count_; break;
        case OperationType::Find:       ++window_find_count_; break;
        case OperationType::Contains:   ++window_contains_count_; break;
        case OperationType::RangeQuery: ++window_range_count_; break;
        case OperationType::Iterate:    ++window_iterate_count_; break;
    }

    window_buffer_[head_index_] = OperationRecord{op, current_time_micros(), result_count};
    head_index_ = (head_index_ + 1) % window_capacity_;
}

WorkloadStats WorkloadMonitor::get_stats() const {
    WorkloadStats stats;
    stats.total_operations = lifetime_operations_;
    stats.window_size = current_size_;

    stats.insert_count = window_insert_count_;
    stats.erase_count = window_erase_count_;
    stats.find_count = window_find_count_;
    stats.contains_count = window_contains_count_;
    stats.range_count = window_range_count_;
    stats.iterate_count = window_iterate_count_;

    if (current_size_ > 0) {
        double total = static_cast<double>(current_size_);
        stats.insert_freq = static_cast<double>(window_insert_count_) / total;
        stats.erase_freq = static_cast<double>(window_erase_count_) / total;
        stats.lookup_freq = static_cast<double>(window_find_count_ + window_contains_count_) / total;
        stats.range_freq = static_cast<double>(window_range_count_) / total;
        stats.iterate_freq = static_cast<double>(window_iterate_count_) / total;

        double writes = static_cast<double>(window_insert_count_ + window_erase_count_);
        double reads = static_cast<double>(window_find_count_ + window_contains_count_ + window_range_count_ + window_iterate_count_);
        stats.read_write_ratio = reads / (writes + 1.0);
    }

    return stats;
}

void WorkloadMonitor::reset() {
    head_index_ = 0;
    current_size_ = 0;
    lifetime_operations_ = 0;

    window_insert_count_ = 0;
    window_erase_count_ = 0;
    window_find_count_ = 0;
    window_contains_count_ = 0;
    window_range_count_ = 0;
    window_iterate_count_ = 0;

    lifetime_insert_count_ = 0;
    lifetime_erase_count_ = 0;
    lifetime_find_count_ = 0;
    lifetime_contains_count_ = 0;
    lifetime_range_count_ = 0;
    lifetime_iterate_count_ = 0;
}

void WorkloadMonitor::resize_window(size_t new_capacity) {
    if (new_capacity < 10) new_capacity = 10;
    window_capacity_ = new_capacity;
    window_buffer_.resize(window_capacity_);
    reset();
}

} // namespace adaptive_ds
