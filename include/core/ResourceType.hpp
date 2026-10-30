#pragma once

#include <string>

namespace deadlock::core {

/**
 * @brief Discrete Resource Type specification.
 */
struct ResourceType {
    int id{0};                ///< Unique resource identifier (0 to m-1)
    std::string name;         ///< Label (e.g. "CPU", "Printer", "Disk")
    int totalCapacity{1};     ///< Total physical units in system (Capacity >= 1)
    int orderRank{0};         ///< Hierarchical rank F(R) for prevention protocol
};

} // namespace deadlock::core
