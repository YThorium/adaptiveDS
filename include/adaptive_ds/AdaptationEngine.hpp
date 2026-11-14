#pragma once

#include "Types.hpp"
#include <string>
#include <vector>
#include <optional>
#include <cmath>

namespace adaptive_ds {

struct AdaptationDecision {
    bool should_adapt{false};
    StructureType target_structure{StructureType::DynamicArray};
    std::string reason;
    std::vector<StructureCostEstimate> candidate_costs;
    double migration_cost{0.0};
    double projected_gain{0.0};
};

class AdaptationEngine {
public:
    explicit AdaptationEngine(const AdaptationConfig& config = AdaptationConfig{});

    // Evaluates whether a switch is warranted based on current state & workload stats
    AdaptationDecision evaluate(
        StructureType current_type,
        size_t element_count,
        const WorkloadStats& stats,
        size_t operations_since_last_switch
    ) const;

    // Estimates operational cost for a structure given element count and workload stats
    StructureCostEstimate estimate_cost(
        StructureType type,
        size_t element_count,
        const WorkloadStats& stats
    ) const;

    // Estimates one-time cost of migrating N elements to target structure
    double estimate_migration_cost(StructureType from, StructureType to, size_t element_count) const;

    const AdaptationConfig& config() const noexcept { return config_; }
    void set_config(const AdaptationConfig& config) { config_ = config; }

private:
    AdaptationConfig config_;
};

} // namespace adaptive_ds
