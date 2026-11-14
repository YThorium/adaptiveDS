#pragma once

#include "Types.hpp"
#include "IDataStructure.hpp"
#include "DynamicArray.hpp"
#include "DoublyLinkedList.hpp"
#include "HashTable.hpp"
#include "AVLTree.hpp"
#include "WorkloadMonitor.hpp"
#include "AdaptationEngine.hpp"
#include "AdaptationHistory.hpp"

#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <stdexcept>
#include <iostream>

namespace adaptive_ds {

template<typename T, 
         typename Compare = std::less<T>, 
         typename Hash = std::hash<T>, 
         typename KeyEqual = std::equal_to<T>>
class AdaptiveDS {
public:
    using Interface = IDataStructure<T, Compare, Hash, KeyEqual>;

    explicit AdaptiveDS(
        StructureType initial_structure = StructureType::DynamicArray,
        const AdaptationConfig& config = AdaptationConfig{}
    ) : config_(config),
        workload_monitor_(config.sliding_window_size),
        adaptation_engine_(config),
        current_type_(initial_structure),
        operation_tick_(0),
        ops_since_last_switch_(0),
        adaptation_id_counter_(0) {
        current_structure_ = create_structure(initial_structure);
    }

    ~AdaptiveDS() = default;

    // Rule of 5: Deep copy & move semantics
    AdaptiveDS(const AdaptiveDS& other)
        : config_(other.config_),
          workload_monitor_(other.workload_monitor_),
          adaptation_engine_(other.adaptation_engine_),
          adaptation_history_(other.adaptation_history_),
          current_type_(other.current_type_),
          operation_tick_(other.operation_tick_),
          ops_since_last_switch_(other.ops_since_last_switch_),
          adaptation_id_counter_(other.adaptation_id_counter_) {
        if (other.current_structure_) {
            current_structure_ = other.current_structure_->clone();
        }
    }

    AdaptiveDS& operator=(const AdaptiveDS& other) {
        if (this != &other) {
            AdaptiveDS temp(other);
            swap(temp);
        }
        return *this;
    }

    AdaptiveDS(AdaptiveDS&& other) noexcept
        : config_(std::move(other.config_)),
          workload_monitor_(std::move(other.workload_monitor_)),
          adaptation_engine_(std::move(other.adaptation_engine_)),
          adaptation_history_(std::move(other.adaptation_history_)),
          current_structure_(std::move(other.current_structure_)),
          current_type_(other.current_type_),
          operation_tick_(other.operation_tick_),
          ops_since_last_switch_(other.ops_since_last_switch_),
          adaptation_id_counter_(other.adaptation_id_counter_) {}

    AdaptiveDS& operator=(AdaptiveDS&& other) noexcept {
        if (this != &other) {
            config_ = std::move(other.config_);
            workload_monitor_ = std::move(other.workload_monitor_);
            adaptation_engine_ = std::move(other.adaptation_engine_);
            adaptation_history_ = std::move(other.adaptation_history_);
            current_structure_ = std::move(other.current_structure_);
            current_type_ = other.current_type_;
            operation_tick_ = other.operation_tick_;
            ops_since_last_switch_ = other.ops_since_last_switch_;
            adaptation_id_counter_ = other.adaptation_id_counter_;
        }
        return *this;
    }

    void swap(AdaptiveDS& other) noexcept {
        std::swap(config_, other.config_);
        std::swap(workload_monitor_, other.workload_monitor_);
        std::swap(adaptation_engine_, other.adaptation_engine_);
        std::swap(adaptation_history_, other.adaptation_history_);
        std::swap(current_structure_, other.current_structure_);
        std::swap(current_type_, other.current_type_);
        std::swap(operation_tick_, other.operation_tick_);
        std::swap(ops_since_last_switch_, other.ops_since_last_switch_);
        std::swap(adaptation_id_counter_, other.adaptation_id_counter_);
    }

    // Unified Data Structure API
    bool insert(const T& val) {
        on_operation(OperationType::Insert);
        bool res = current_structure_->insert(val);
        check_and_adapt();
        return res;
    }

    bool insert(T&& val) {
        on_operation(OperationType::Insert);
        bool res = current_structure_->insert(std::move(val));
        check_and_adapt();
        return res;
    }

    bool erase(const T& val) {
        on_operation(OperationType::Erase);
        bool res = current_structure_->erase(val);
        check_and_adapt();
        return res;
    }

    bool contains(const T& val) const {
        const_cast<AdaptiveDS*>(this)->on_operation(OperationType::Contains);
        bool res = current_structure_->contains(val);
        const_cast<AdaptiveDS*>(this)->check_and_adapt();
        return res;
    }

    const T* find(const T& val) const {
        const_cast<AdaptiveDS*>(this)->on_operation(OperationType::Find);
        const T* res = current_structure_->find(val);
        const_cast<AdaptiveDS*>(this)->check_and_adapt();
        return res;
    }

    std::vector<T> range(const T& low, const T& high) const {
        const_cast<AdaptiveDS*>(this)->on_operation(OperationType::RangeQuery);
        std::vector<T> res = current_structure_->range(low, high);
        const_cast<AdaptiveDS*>(this)->check_and_adapt();
        return res;
    }

    size_t size() const noexcept {
        return current_structure_ ? current_structure_->size() : 0;
    }

    bool empty() const noexcept {
        return size() == 0;
    }

    void clear() {
        if (current_structure_) {
            current_structure_->clear();
        }
    }

    std::vector<T> to_vector() const {
        const_cast<AdaptiveDS*>(this)->on_operation(OperationType::Iterate);
        return current_structure_ ? current_structure_->to_vector() : std::vector<T>{};
    }

    // Introspection
    StructureType current_structure_type() const noexcept {
        return current_type_;
    }

    const char* current_structure_name() const noexcept {
        return to_string(current_type_);
    }

    size_t memory_bytes() const noexcept {
        return sizeof(*this) + (current_structure_ ? current_structure_->memory_bytes() : 0);
    }

    WorkloadStats get_stats() const {
        return workload_monitor_.get_stats();
    }

    const std::vector<AdaptationRecord>& get_adaptation_history() const noexcept {
        return adaptation_history_.records();
    }

    const AdaptationHistory& history() const noexcept {
        return adaptation_history_;
    }

    bool export_history_csv(const std::string& filepath) const {
        return adaptation_history_.export_to_csv(filepath);
    }

    // Configuration & control
    const AdaptationConfig& config() const noexcept {
        return config_;
    }

    void set_config(const AdaptationConfig& cfg) {
        config_ = cfg;
        workload_monitor_.resize_window(config_.sliding_window_size);
        adaptation_engine_.set_config(config_);
    }

    void set_adaptive_enabled(bool enabled) noexcept {
        config_.enable_automatic_adaptation = enabled;
    }

    void reset_stats() {
        workload_monitor_.reset();
        ops_since_last_switch_ = 0;
    }

    // Explicit migration
    bool force_adapt(StructureType target_type) {
        if (target_type == current_type_) {
            return false;
        }
        perform_migration(target_type, "Forced adaptation requested by user");
        return true;
    }

    // Evaluates current conditions and triggers migration if criteria are satisfied
    bool check_and_adapt() {
        if (!config_.enable_automatic_adaptation) {
            return false;
        }

        // Fast path: cooldown period
        if (ops_since_last_switch_ < config_.cooldown_operations) {
            return false;
        }

        // Fast path: minimum operations required
        if (workload_monitor_.current_window_size() < config_.min_operations_before_adaptation) {
            return false;
        }

        // Periodic evaluation check to minimize telemetry & engine evaluation overhead
        if (config_.check_frequency > 1 && (operation_tick_ % config_.check_frequency) != 0) {
            return false;
        }

        WorkloadStats stats = workload_monitor_.get_stats();
        AdaptationDecision decision = adaptation_engine_.evaluate(
            current_type_,
            size(),
            stats,
            ops_since_last_switch_
        );

        if (decision.should_adapt) {
            perform_migration(decision.target_structure, decision.reason, decision.candidate_costs);
            return true;
        }

        return false;
    }

private:
    void on_operation(OperationType op) {
        ++operation_tick_;
        ++ops_since_last_switch_;
        workload_monitor_.record_operation(op);
    }

    static std::unique_ptr<Interface> create_structure(StructureType type) {
        switch (type) {
            case StructureType::DynamicArray:
                return std::make_unique<DynamicArray<T, Compare, Hash, KeyEqual>>();
            case StructureType::DoublyLinkedList:
                return std::make_unique<DoublyLinkedList<T, Compare, Hash, KeyEqual>>();
            case StructureType::HashTable:
                return std::make_unique<HashTable<T, Compare, Hash, KeyEqual>>();
            case StructureType::AVLTree:
                return std::make_unique<AVLTree<T, Compare, Hash, KeyEqual>>();
        }
        return std::make_unique<DynamicArray<T, Compare, Hash, KeyEqual>>();
    }

    void perform_migration(
        StructureType target_type, 
        const std::string& reason, 
        const std::vector<StructureCostEstimate>& costs = {}
    ) {
        auto start_time = std::chrono::high_resolution_clock::now();

        size_t initial_size = current_structure_->size();
        std::vector<T> elements = current_structure_->to_vector();

        // Create new container
        std::unique_ptr<Interface> new_struct = create_structure(target_type);

        // Migrate all elements safely
        for (auto& item : elements) {
            new_struct->insert(std::move(item));
        }

        // Invariant verification
        if (new_struct->size() != initial_size) {
            throw std::runtime_error("AdaptiveDS migration failed element preservation invariant: expected " + 
                                     std::to_string(initial_size) + " but got " + 
                                     std::to_string(new_struct->size()));
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        double duration_us = std::chrono::duration<double, std::micro>(end_time - start_time).count();

        // Safe pointer swap
        StructureType old_type = current_type_;
        current_structure_ = std::move(new_struct);
        current_type_ = target_type;
        ops_since_last_switch_ = 0;

        // Record adaptation event
        AdaptationRecord record;
        record.id = ++adaptation_id_counter_;
        record.timestamp_us = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
        );
        record.operation_tick = operation_tick_;
        record.from_structure = old_type;
        record.to_structure = target_type;
        record.element_count = initial_size;
        record.migration_duration_us = duration_us;
        record.reason = reason;
        record.workload_snapshot = workload_monitor_.get_stats();
        record.candidate_costs = costs;

        adaptation_history_.record_switch(record);

        if (config_.verbose_logging) {
            std::cout << "[AdaptiveDS Auto-Adapt] " << to_string(old_type) 
                      << " -> " << to_string(target_type) 
                      << " (took " << duration_us << " us)\n"
                      << "  Reason: " << reason << std::endl;
        }
    }

    AdaptationConfig config_;
    WorkloadMonitor workload_monitor_;
    AdaptationEngine adaptation_engine_;
    AdaptationHistory adaptation_history_;

    std::unique_ptr<Interface> current_structure_;
    StructureType current_type_;

    size_t operation_tick_{0};
    size_t ops_since_last_switch_{0};
    uint64_t adaptation_id_counter_{0};
};

} // namespace adaptive_ds
