#include "adaptive_ds/AdaptationHistory.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace adaptive_ds {

void AdaptationHistory::record_switch(const AdaptationRecord& record) {
    history_.push_back(record);
}

bool AdaptationHistory::export_to_csv(const std::string& filepath) const {
    std::ofstream ofs(filepath);
    if (!ofs.is_open()) {
        return false;
    }

    // CSV Header
    ofs << "id,timestamp_us,operation_tick,from_structure,to_structure,element_count,"
        << "migration_duration_us,lookup_freq,range_freq,insert_freq,erase_freq,"
        << "cost_dyn_array,cost_dlist,cost_hashtable,cost_avl,reason\n";

    for (const auto& rec : history_) {
        double cost_arr = 0.0, cost_list = 0.0, cost_hash = 0.0, cost_avl = 0.0;
        for (const auto& c : rec.candidate_costs) {
            if (c.type == StructureType::DynamicArray) cost_arr = c.total_estimated_cost;
            else if (c.type == StructureType::DoublyLinkedList) cost_list = c.total_estimated_cost;
            else if (c.type == StructureType::HashTable) cost_hash = c.total_estimated_cost;
            else if (c.type == StructureType::AVLTree) cost_avl = c.total_estimated_cost;
        }

        // Escape reason quotes
        std::string escaped_reason = rec.reason;
        size_t pos = 0;
        while ((pos = escaped_reason.find('"', pos)) != std::string::npos) {
            escaped_reason.replace(pos, 1, "\"\"");
            pos += 2;
        }

        ofs << rec.id << ","
            << rec.timestamp_us << ","
            << rec.operation_tick << ","
            << to_string(rec.from_structure) << ","
            << to_string(rec.to_structure) << ","
            << rec.element_count << ","
            << std::fixed << std::setprecision(2) << rec.migration_duration_us << ","
            << std::setprecision(4) << rec.workload_snapshot.lookup_freq << ","
            << rec.workload_snapshot.range_freq << ","
            << rec.workload_snapshot.insert_freq << ","
            << rec.workload_snapshot.erase_freq << ","
            << std::setprecision(2) << cost_arr << ","
            << cost_list << ","
            << cost_hash << ","
            << cost_avl << ","
            << "\"" << escaped_reason << "\"\n";
    }

    return true;
}

std::string AdaptationHistory::summary() const {
    std::ostringstream ss;
    ss << "=== AdaptiveDS Adaptation History Summary ===\n";
    ss << "Total adaptation switches: " << history_.size() << "\n\n";

    for (size_t i = 0; i < history_.size(); ++i) {
        const auto& rec = history_[i];
        ss << "[" << (i + 1) << "] OpTick: " << rec.operation_tick
           << " | " << to_string(rec.from_structure) << " -> " << to_string(rec.to_structure)
           << " | N=" << rec.element_count
           << " | Migration: " << std::fixed << std::setprecision(2) << rec.migration_duration_us << " us\n"
           << "    Reason: " << rec.reason << "\n";
    }
    return ss.str();
}

void AdaptationHistory::print_log(std::ostream& os) const {
    os << summary();
}

} // namespace adaptive_ds
