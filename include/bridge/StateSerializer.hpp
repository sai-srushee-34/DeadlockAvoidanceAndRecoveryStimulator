#pragma once

#include "core/SystemState.hpp"
#include "core/ResourceManager.hpp"
#include "detection/DetectionEngine.hpp"
#include "recovery/RecoveryEngine.hpp"
#include "bridge/Json.hpp"

#include <string>
#include <vector>

namespace deadlock::bridge {

/**
 * @brief Event structure for simulation audit history.
 */
struct SimulationEvent {
    std::string id;
    std::string timestamp;
    std::string category;  // "SYSTEM", "SCENARIO", "DETECTION", "RECOVERY", "PREVENTION", "ALLOCATION", "PROCESS", "RESOURCE"
    std::string severity;  // "INFO", "SUCCESS", "WARNING", "ERROR"
    std::string message;
    JsonValue metadata;
};

/**
 * @brief Serializes simulation state, metrics, detection traces, and recovery reports into JSON.
 */
class StateSerializer {
public:
    static JsonValue serializeState(const core::SystemState& state, 
                                    prevention::PreventionPolicy policy,
                                    const std::string& scenarioId = "custom",
                                    const std::string& scenarioName = "Custom Configuration",
                                    const std::string& description = "");

    static JsonValue serializeDetection(const detection::DetectionResult& result);
    static JsonValue serializeRecovery(const recovery::RecoveryResult& result);
    static JsonValue serializeEvent(const SimulationEvent& evt);
    static JsonValue serializeEvents(const std::vector<SimulationEvent>& events);
    static JsonValue serializeRejection(const core::RejectionRecord& rec);
};

} // namespace deadlock::bridge
