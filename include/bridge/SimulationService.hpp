#pragma once

#include "core/ResourceManager.hpp"
#include "bridge/StateSerializer.hpp"

#include <string>
#include <vector>
#include <mutex>
#include <memory>

namespace deadlock::bridge {

/**
 * @brief High-level application service managing the authoritative simulation engine,
 * demonstration scenarios, and event audit history.
 */
class SimulationService {
public:
    SimulationService();
    ~SimulationService() = default;

    // --- Scenario & State Management ---
    JsonValue getState();
    JsonValue loadScenario(const std::string& scenarioId);
    JsonValue resetCurrentScenario();
    JsonValue getAvailableScenarios();

    // --- Entity Mutations ---
    struct AddProcessParams {
        std::string name;
        int priority{5};
        int cost{50};
        std::vector<int> maxDemand;
        std::vector<int> initialAlloc;
    };

    struct AddResourceParams {
        std::string name;
        int capacity{1};
        int orderRank{1};
    };

    struct SubmitRequestParams {
        int processId{-1};
        std::vector<int> requestVector;
    };

    struct ReleaseResourcesParams {
        int processId{-1};
        std::vector<int> releaseVector;
    };

    JsonValue addProcess(const AddProcessParams& params, std::string& outError);
    JsonValue deleteProcess(int processId, std::string& outError);
    JsonValue clearProcesses(std::string& outError);
    JsonValue addResource(const AddResourceParams& params, std::string& outError);
    JsonValue deleteResource(int resourceId, std::string& outError);
    JsonValue clearResources(std::string& outError);
    JsonValue clearAll(std::string& outError);
    JsonValue submitRequest(const SubmitRequestParams& params, std::string& outError);
    JsonValue releaseResources(const ReleaseResourcesParams& params, std::string& outError);

    // --- Prevention, Detection & Recovery ---
    JsonValue setPreventionPolicy(const std::string& policyStr, std::string& outError);
    JsonValue runDetection();
    JsonValue recoverVictimManual(int victimProcessId, std::string& outError);
    JsonValue recoverVictimAuto(const std::string& policyStr, std::string& outError);
    JsonValue getVictimCandidates();

    // --- Audit Event History ---
    JsonValue getEvents();
    void clearEvents();
    void addEvent(const std::string& category, const std::string& message, const std::string& severity = "INFO", JsonValue metadata = JsonValue());

private:
    mutable std::recursive_mutex mutex_;
    core::ResourceManager engine_;
    std::string activeScenarioId_{"standard_safe"};
    std::string activeScenarioName_{"Standard Safe State (3P, 3R)"};
    std::string activeScenarioDesc_{"System operating normally with sufficient available headroom. Banker algorithm safe sequence exists: P1 -> P0 -> P2."};
    std::vector<SimulationEvent> eventLog_;

    void setupScenario(const std::string& scenarioId);
    static std::string currentTimestamp();
};

} // namespace deadlock::bridge
