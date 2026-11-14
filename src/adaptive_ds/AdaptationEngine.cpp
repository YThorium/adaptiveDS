#include "adaptive_ds/AdaptationEngine.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace adaptive_ds {

AdaptationEngine::AdaptationEngine(const AdaptationConfig& config)
    : config_(config) {}

StructureCostEstimate AdaptationEngine::estimate_cost(
    StructureType type,
    size_t element_count,
    const WorkloadStats& stats
) const {
    StructureCostEstimate est;
    est.type = type;

    double n = static_cast<double>(element_count > 0 ? element_count : 1);
    double log_n = std::log2(n + 1.0);

    // Weights calibrated based on operation time profiles (normalized baseline)
    switch (type) {
        case StructureType::DynamicArray: {
            // Contiguous cache locality bonus: 0.25 factor for scanning
            // Lookups: linear scan O(N)
            est.lookup_cost = stats.lookup_freq * (0.25 * n);
            // Inserts: uniqueness scan O(N) + amortized append O(1)
            est.insert_cost = stats.insert_freq * (0.25 * n + 0.5);
            // Erase: scan O(N) + shift elements O(N)
            est.erase_cost = stats.erase_freq * (0.50 * n);
            // Range: scan O(N) + small sort overhead
            est.range_cost = stats.range_freq * (0.35 * n + 0.1 * log_n);

            // Small dataset cache bonus: When N is small, array fits entirely in L1/L2 cache
            if (element_count <= config_.small_dataset_threshold) {
                est.lookup_cost *= 0.3;
                est.insert_cost *= 0.4;
                est.erase_cost *= 0.4;
                est.range_cost *= 0.3;
            }
            break;
        }

        case StructureType::DoublyLinkedList: {
            // Pointer chasing / no cache locality: penalty factor 1.5x
            // Lookups: linear traversal O(N)
            est.lookup_cost = stats.lookup_freq * (1.20 * n);
            // Inserts: linear scan for uniqueness O(N) + O(1) node allocation
            est.insert_cost = stats.insert_freq * (1.20 * n + 2.0);
            // Erase: linear scan to node O(N) + O(1) pointer rewire
            est.erase_cost = stats.erase_freq * (1.20 * n + 1.0);
            // Range: scan all nodes O(N) + sort matches
            est.range_cost = stats.range_freq * (1.50 * n + 0.5 * log_n);
            break;
        }

        case StructureType::HashTable: {
            // Hash table has O(1) average lookup, insert, erase
            // Constant cost for hashing and bucket traversal
            est.lookup_cost = stats.lookup_freq * 1.5;
            est.insert_cost = stats.insert_freq * 2.5; // hash + heap node allocation
            est.erase_cost = stats.erase_freq * 2.0;  // hash + node deletion

            // Range query is Hash Table's Achilles' heel: must scan all buckets and sort
            est.range_cost = stats.range_freq * (2.0 * n + 1.0 * n * log_n * 0.1);
            break;
        }

        case StructureType::AVLTree: {
            // O(log N) lookup, insert, erase
            est.lookup_cost = stats.lookup_freq * (1.8 * log_n);
            // Insert with tree traversal + rotations
            est.insert_cost = stats.insert_freq * (2.8 * log_n + 1.5);
            // Erase with replacement + rotations
            est.erase_cost = stats.erase_freq * (3.0 * log_n + 1.5);
            // Range query is AVL Tree's strength: sub-tree bounds traversal O(log N + K)
            est.range_cost = stats.range_freq * (1.2 * log_n + 0.15 * std::min(n, 20.0));
            break;
        }
    }

    est.total_estimated_cost = est.lookup_cost + est.insert_cost + est.erase_cost + est.range_cost;
    return est;
}

double AdaptationEngine::estimate_migration_cost(
    StructureType /*from*/,
    StructureType to,
    size_t element_count
) const {
    double n = static_cast<double>(element_count);
    switch (to) {
        case StructureType::DynamicArray:
            // Contiguous memcpy / reserve: lowest migration cost
            return 0.6 * n + 10.0;
        case StructureType::DoublyLinkedList:
            // Per-element node allocation
            return 1.4 * n + 15.0;
        case StructureType::HashTable:
            // Per-element hashing + bucket insertion + initial table
            return 2.0 * n + 30.0;
        case StructureType::AVLTree:
            // Per-element insertion with rebalancing: O(N log N)
            return (1.8 * std::log2(n + 2.0) + 1.0) * n + 25.0;
    }
    return 2.0 * n;
}

AdaptationDecision AdaptationEngine::evaluate(
    StructureType current_type,
    size_t element_count,
    const WorkloadStats& stats,
    size_t operations_since_last_switch
) const {
    AdaptationDecision decision;
    decision.should_adapt = false;
    decision.target_structure = current_type;

    // Check minimum operations before adaptation
    if (stats.window_size < config_.min_operations_before_adaptation) {
        decision.reason = "Window size (" + std::to_string(stats.window_size) + 
                          ") below minimum required threshold (" + 
                          std::to_string(config_.min_operations_before_adaptation) + ")";
        return decision;
    }

    // Check cooldown period
    if (operations_since_last_switch < config_.cooldown_operations) {
        decision.reason = "In cooldown: " + std::to_string(operations_since_last_switch) + 
                          " ops since last switch (cooldown threshold: " + 
                          std::to_string(config_.cooldown_operations) + ")";
        return decision;
    }

    // Compute costs for all candidate structures
    std::vector<StructureType> candidates = {
        StructureType::DynamicArray,
        StructureType::DoublyLinkedList,
        StructureType::HashTable,
        StructureType::AVLTree
    };

    StructureCostEstimate current_est = estimate_cost(current_type, element_count, stats);
    StructureCostEstimate best_est = current_est;
    StructureType best_type = current_type;

    decision.candidate_costs.reserve(candidates.size());
    for (StructureType c : candidates) {
        StructureCostEstimate est = estimate_cost(c, element_count, stats);
        decision.candidate_costs.push_back(est);
        if (est.total_estimated_cost < best_est.total_estimated_cost) {
            best_est = est;
            best_type = c;
        }
    }

    // If current structure is already best, no need to adapt
    if (best_type == current_type) {
        decision.reason = "Current structure (" + std::string(to_string(current_type)) + 
                          ") remains optimal (cost: " + std::to_string(current_est.total_estimated_cost) + ")";
        return decision;
    }

    // Relative improvement check
    double cost_reduction = current_est.total_estimated_cost - best_est.total_estimated_cost;
    double relative_improvement = cost_reduction / (current_est.total_estimated_cost + 1e-6);

    if (relative_improvement < config_.improvement_threshold) {
        std::ostringstream ss;
        ss << "Candidate " << to_string(best_type) << " improvement (" 
           << std::fixed << std::setprecision(1) << (relative_improvement * 100.0)
           << "%) is below hysteresis threshold (" 
           << (config_.improvement_threshold * 100.0) << "%)";
        decision.reason = ss.str();
        return decision;
    }

    // Migration cost amortization check
    double migration_cost = estimate_migration_cost(current_type, best_type, element_count);
    double projected_savings = cost_reduction * static_cast<double>(config_.projection_horizon);

    decision.migration_cost = migration_cost;
    decision.projected_gain = projected_savings - migration_cost;

    if (projected_savings <= migration_cost) {
        std::ostringstream ss;
        ss << "Projected savings (" << projected_savings 
           << ") over horizon " << config_.projection_horizon 
           << " does not amortize migration cost (" << migration_cost << ")";
        decision.reason = ss.str();
        return decision;
    }

    // Adaptation approved!
    decision.should_adapt = true;
    decision.target_structure = best_type;

    std::ostringstream ss;
    ss << "Adapting " << to_string(current_type) << " -> " << to_string(best_type)
       << " [N=" << element_count
       << ", Lookup=" << std::fixed << std::setprecision(1) << (stats.lookup_freq * 100.0) << "%"
       << ", Range=" << (stats.range_freq * 100.0) << "%"
       << ", Ins/Del=" << ((stats.insert_freq + stats.erase_freq) * 100.0) << "%"
       << " | Current cost: " << std::setprecision(2) << current_est.total_estimated_cost
       << " -> Best cost: " << best_est.total_estimated_cost
       << " (" << (relative_improvement * 100.0) << "% reduction)"
       << " | Migration cost: " << migration_cost
       << " | Net projected gain: " << decision.projected_gain << "]";
    decision.reason = ss.str();

    return decision;
}

} // namespace adaptive_ds
