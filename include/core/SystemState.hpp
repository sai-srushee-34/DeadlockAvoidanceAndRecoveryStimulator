#pragma once

#include "Process.hpp"
#include "ResourceType.hpp"

#include <vector>
#include <string>
#include <sstream>

namespace deadlock::core {

/**
 * @brief Authoritative snapshot and matrix representation of system state.
 */
struct SystemState {
    int simulationStep{0};
    bool maxDemandModelEnabled{false};

    std::vector<ResourceType> resources;           ///< Resource definitions (m)
    std::vector<Process> processes;                 ///< Process definitions (n)

    std::vector<int> available;                     ///< Available instances vector (m)
    std::vector<std::vector<int>> allocation;       ///< Allocation matrix (n x m)
    std::vector<std::vector<int>> request;          ///< Outstanding Request matrix (n x m)
    std::vector<std::vector<int>> maxDemand;        ///< Maximum Demand matrix (n x m)
    std::vector<std::vector<int>> need;             ///< Remaining Need matrix (n x m)

    int numProcesses() const {
        return static_cast<int>(processes.size());
    }

    int numResources() const {
        return static_cast<int>(resources.size());
    }

    /**
     * @brief Formally verify all Operating Systems conservation laws and matrix invariants.
     * @param outError Populated with description if any invariant is violated.
     * @return true if all invariants hold; false otherwise.
     */
    bool verifyInvariants(std::string& outError) const {
        const int n = numProcesses();
        const int m = numResources();

        // 1. Validate vector and matrix dimensions
        if (static_cast<int>(available.size()) != m) {
            outError = "Dimension error: Available vector size (" + std::to_string(available.size()) +
                       ") does not match resource count (" + std::to_string(m) + ").";
            return false;
        }

        if (static_cast<int>(allocation.size()) != n ||
            static_cast<int>(request.size()) != n ||
            static_cast<int>(maxDemand.size()) != n ||
            static_cast<int>(need.size()) != n) {
            outError = "Dimension error: Matrix row count does not match process count (" + std::to_string(n) + ").";
            return false;
        }

        for (int i = 0; i < n; ++i) {
            if (static_cast<int>(allocation[i].size()) != m ||
                static_cast<int>(request[i].size()) != m ||
                static_cast<int>(maxDemand[i].size()) != m ||
                static_cast<int>(need[i].size()) != m) {
                outError = "Dimension error: Matrix row " + std::to_string(i) + " width does not match resource count (" + std::to_string(m) + ").";
                return false;
            }
        }

        // 2. Element-wise non-negativity
        for (int j = 0; j < m; ++j) {
            if (resources[j].totalCapacity < 0) {
                outError = "Invariant violation: Resource '" + resources[j].name + "' has negative capacity.";
                return false;
            }
            if (available[j] < 0) {
                outError = "Invariant violation: Resource '" + resources[j].name + "' has negative available instances (" + std::to_string(available[j]) + ").";
                return false;
            }
        }

        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                if (allocation[i][j] < 0) {
                    outError = "Invariant violation: Allocation[" + std::to_string(i) + "][" + std::to_string(j) + "] is negative (" + std::to_string(allocation[i][j]) + ").";
                    return false;
                }
                if (request[i][j] < 0) {
                    outError = "Invariant violation: Request[" + std::to_string(i) + "][" + std::to_string(j) + "] is negative (" + std::to_string(request[i][j]) + ").";
                    return false;
                }
                if (maxDemand[i][j] < 0) {
                    outError = "Invariant violation: MaxDemand[" + std::to_string(i) + "][" + std::to_string(j) + "] is negative (" + std::to_string(maxDemand[i][j]) + ").";
                    return false;
                }
                if (need[i][j] < 0) {
                    outError = "Invariant violation: Need[" + std::to_string(i) + "][" + std::to_string(j) + "] is negative (" + std::to_string(need[i][j]) + ").";
                    return false;
                }
            }
        }

        // 3. Conservation of Resource Capacity: Available[j] + Sum(Allocation[i][j]) == TotalCapacity[j]
        for (int j = 0; j < m; ++j) {
            int allocatedSum = 0;
            for (int i = 0; i < n; ++i) {
                allocatedSum += allocation[i][j];
            }
            if (allocatedSum + available[j] != resources[j].totalCapacity) {
                outError = "Conservation violation for resource '" + resources[j].name + "': TotalCapacity (" +
                           std::to_string(resources[j].totalCapacity) + ") != Allocated (" +
                           std::to_string(allocatedSum) + ") + Available (" +
                           std::to_string(available[j]) + ").";
                return false;
            }
        }

        // 4. Max Demand and Need Consistency (when model is enabled)
        if (maxDemandModelEnabled) {
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < m; ++j) {
                    if (allocation[i][j] > maxDemand[i][j]) {
                        outError = "Max demand violation: Process P" + std::to_string(i) + " allocated " +
                                   std::to_string(allocation[i][j]) + " > MaxDemand " +
                                   std::to_string(maxDemand[i][j]) + " for resource " + resources[j].name + ".";
                        return false;
                    }
                    if (need[i][j] != maxDemand[i][j] - allocation[i][j]) {
                        outError = "Need consistency violation: Process P" + std::to_string(i) + " Need (" +
                                   std::to_string(need[i][j]) + ") != Max (" +
                                   std::to_string(maxDemand[i][j]) + ") - Alloc (" +
                                   std::to_string(allocation[i][j]) + ") for resource " + resources[j].name + ".";
                        return false;
                    }
                    if (maxDemand[i][j] > resources[j].totalCapacity) {
                        outError = "Max demand exceeds total capacity: Process P" + std::to_string(i) +
                                   " claims " + std::to_string(maxDemand[i][j]) + " > Capacity " +
                                   std::to_string(resources[j].totalCapacity) + " for resource " + resources[j].name + ".";
                        return false;
                    }
                }
            }
        }

        return true;
    }
};

} // namespace deadlock::core
