#pragma once

#include "Types.hpp"
#include <vector>
#include <string>
#include <ostream>

namespace adaptive_ds {

class AdaptationHistory {
public:
    AdaptationHistory() = default;

    void record_switch(const AdaptationRecord& record);

    const std::vector<AdaptationRecord>& records() const noexcept {
        return history_;
    }

    size_t count() const noexcept {
        return history_.size();
    }

    bool empty() const noexcept {
        return history_.empty();
    }

    void clear() {
        history_.clear();
    }

    // Export history to CSV file format
    bool export_to_csv(const std::string& filepath) const;

    // Format human-readable summary
    std::string summary() const;

    // Detailed log of all adaptation decisions
    void print_log(std::ostream& os) const;

private:
    std::vector<AdaptationRecord> history_;
};

} // namespace adaptive_ds
