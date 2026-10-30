#include "bridge/ApiRouter.hpp"
#include "bridge/Json.hpp"

#include <iostream>

namespace deadlock::bridge {

namespace {

int parseEntityId(const JsonValue& val, int def = -1) {
    if (val.isNumber()) {
        return val.asInt(def);
    }
    if (val.isString()) {
        std::string s = val.asString();
        if (!s.empty() && (s[0] == 'P' || s[0] == 'p' || s[0] == 'R' || s[0] == 'r')) {
            s = s.substr(1);
        }
        try {
            return std::stoi(s);
        } catch (...) {
            return def;
        }
    }
    return def;
}

std::vector<int> parseVector(const JsonValue& val) {
    std::vector<int> vec;
    if (val.isArray()) {
        for (const auto& item : val.asArray()) {
            vec.push_back(item.asInt(0));
        }
    }
    return vec;
}

} // namespace

void ApiRouter::registerRoutes(HttpServer& server, std::shared_ptr<SimulationService> service) {

    // 1. GET /api/state - Retrieve current system state
    server.setRoute("GET", "/api/state", [service](const HttpRequest&) {
        JsonValue state = service->getState();
        return HttpResponse::Ok(state.serialize());
    });

    // 2. POST /api/processes - Register a new process
    server.setRoute("POST", "/api/processes", [service](const HttpRequest& req) {
        try {
            JsonValue body = JsonValue::parse(req.body);
            if (!body.isObject()) {
                return HttpResponse::BadRequest("Malformed JSON request body.");
            }

            SimulationService::AddProcessParams params;
            params.name = body.get("name").asString();
            params.priority = body.get("priority").asInt(5);
            params.cost = body.get("cost").asInt(50);
            params.maxDemand = parseVector(body.get("maxDemand"));
            params.initialAlloc = parseVector(body.get("initialAlloc"));

            std::string err;
            JsonValue state = service->addProcess(params, err);
            if (!err.empty()) {
                return HttpResponse::Unprocessable(err);
            }

            return HttpResponse::Ok(state.serialize());
        } catch (const std::exception& ex) {
            return HttpResponse::BadRequest(std::string("Invalid JSON: ") + ex.what());
        }
    });

    // 3. POST /api/resources - Define a new resource class
    server.setRoute("POST", "/api/resources", [service](const HttpRequest& req) {
        try {
            JsonValue body = JsonValue::parse(req.body);
            if (!body.isObject()) {
                return HttpResponse::BadRequest("Malformed JSON request body.");
            }

            SimulationService::AddResourceParams params;
            params.name = body.get("name").asString();
            params.capacity = body.get("capacity").asInt(1);
            params.orderRank = body.get("orderRank").asInt(1);

            std::string err;
            JsonValue state = service->addResource(params, err);
            if (!err.empty()) {
                return HttpResponse::Unprocessable(err);
            }

            return HttpResponse::Ok(state.serialize());
        } catch (const std::exception& ex) {
            return HttpResponse::BadRequest(std::string("Invalid JSON: ") + ex.what());
        }
    });

    // 4. POST /api/requests - Submit a resource request
    server.setRoute("POST", "/api/requests", [service](const HttpRequest& req) {
        try {
            JsonValue body = JsonValue::parse(req.body);
            if (!body.isObject()) {
                return HttpResponse::BadRequest("Malformed JSON request body.");
            }

            SimulationService::SubmitRequestParams params;
            params.processId = parseEntityId(body.get("processId"));

            if (body.has("requestVector")) {
                params.requestVector = parseVector(body.get("requestVector"));
            } else if (body.has("resourceId") && body.has("units")) {
                // Single resource claim format
                int rId = parseEntityId(body.get("resourceId"));
                int units = body.get("units").asInt(1);
                
                // Get resource count
                JsonValue curState = service->getState();
                int rCount = curState.get("resources").asArray().size();
                params.requestVector.assign(rCount, 0);
                if (rId >= 0 && rId < rCount) {
                    params.requestVector[rId] = units;
                }
            } else {
                return HttpResponse::BadRequest("Request must contain 'requestVector' or ('resourceId' and 'units').");
            }

            std::string err;
            JsonValue state = service->submitRequest(params, err);
            if (!err.empty()) {
                return HttpResponse::Unprocessable(err);
            }

            return HttpResponse::Ok(state.serialize());
        } catch (const std::exception& ex) {
            return HttpResponse::BadRequest(std::string("Invalid JSON: ") + ex.what());
        }
    });

    // 5. POST /api/release - Release held resources
    server.setRoute("POST", "/api/release", [service](const HttpRequest& req) {
        try {
            JsonValue body = JsonValue::parse(req.body);
            if (!body.isObject()) {
                return HttpResponse::BadRequest("Malformed JSON request body.");
            }

            SimulationService::ReleaseResourcesParams params;
            params.processId = parseEntityId(body.get("processId"));

            if (body.has("releaseVector")) {
                params.releaseVector = parseVector(body.get("releaseVector"));
            } else if (body.has("resourceId") && body.has("units")) {
                int rId = parseEntityId(body.get("resourceId"));
                int units = body.get("units").asInt(1);

                JsonValue curState = service->getState();
                int rCount = curState.get("resources").asArray().size();
                params.releaseVector.assign(rCount, 0);
                if (rId >= 0 && rId < rCount) {
                    params.releaseVector[rId] = units;
                }
            } else {
                return HttpResponse::BadRequest("Request must contain 'releaseVector' or ('resourceId' and 'units').");
            }

            std::string err;
            JsonValue state = service->releaseResources(params, err);
            if (!err.empty()) {
                return HttpResponse::Unprocessable(err);
            }

            return HttpResponse::Ok(state.serialize());
        } catch (const std::exception& ex) {
            return HttpResponse::BadRequest(std::string("Invalid JSON: ") + ex.what());
        }
    });

    // 6. POST /api/prevention/policy - Configure active prevention policy
    server.setRoute("POST", "/api/prevention/policy", [service](const HttpRequest& req) {
        try {
            JsonValue body = JsonValue::parse(req.body);
            std::string policy = body.get("policy").asString("NONE");

            std::string err;
            JsonValue res = service->setPreventionPolicy(policy, err);
            if (!err.empty()) {
                return HttpResponse::Unprocessable(err);
            }

            return HttpResponse::Ok(res.serialize());
        } catch (const std::exception& ex) {
            return HttpResponse::BadRequest(std::string("Invalid JSON: ") + ex.what());
        }
    });

    // 7. POST /api/detection/run - Execute multi-instance deadlock detection
    server.setRoute("POST", "/api/detection/run", [service](const HttpRequest&) {
        JsonValue report = service->runDetection();
        return HttpResponse::Ok(report.serialize());
    });

    // 8. POST /api/recovery/terminate - Terminate a selected victim process
    server.setRoute("POST", "/api/recovery/terminate", [service](const HttpRequest& req) {
        try {
            JsonValue body = JsonValue::parse(req.body);
            std::string policy = body.get("policy").asString("MANUAL");

            std::string err;
            JsonValue res;

            if (policy == "MANUAL" || body.has("victimProcessId")) {
                int victimId = parseEntityId(body.get("victimProcessId"));
                res = service->recoverVictimManual(victimId, err);
            } else {
                res = service->recoverVictimAuto(policy, err);
            }

            if (!err.empty()) {
                return HttpResponse::Unprocessable(err);
            }

            return HttpResponse::Ok(res.serialize());
        } catch (const std::exception& ex) {
            return HttpResponse::BadRequest(std::string("Invalid JSON: ") + ex.what());
        }
    });

    // 8b. GET /api/recovery/candidates - Get heuristic victim rankings
    server.setRoute("GET", "/api/recovery/candidates", [service](const HttpRequest&) {
        JsonValue candidates = service->getVictimCandidates();
        return HttpResponse::Ok(candidates.serialize());
    });

    // 9. GET /api/events - Retrieve simulation event history
    server.setRoute("GET", "/api/events", [service](const HttpRequest&) {
        JsonValue events = service->getEvents();
        return HttpResponse::Ok(events.serialize());
    });

    // 9b. POST /api/events/clear - Clear simulation audit log
    server.setRoute("POST", "/api/events/clear", [service](const HttpRequest&) {
        service->clearEvents();
        JsonValue res;
        res["success"] = true;
        res["message"] = "Event log cleared.";
        return HttpResponse::Ok(res.serialize());
    });

    // 10. POST /api/scenario/load - Load a predefined scenario
    server.setRoute("POST", "/api/scenario/load", [service](const HttpRequest& req) {
        try {
            JsonValue body = JsonValue::parse(req.body);
            std::string scenarioId = body.get("scenarioId").asString("standard_safe");
            JsonValue state = service->loadScenario(scenarioId);
            return HttpResponse::Ok(state.serialize());
        } catch (const std::exception& ex) {
            return HttpResponse::BadRequest(std::string("Invalid JSON: ") + ex.what());
        }
    });

    // 10b. POST /api/scenario/reset - Reset current scenario to baseline
    server.setRoute("POST", "/api/scenario/reset", [service](const HttpRequest&) {
        JsonValue state = service->resetCurrentScenario();
        return HttpResponse::Ok(state.serialize());
    });

    // 10c. POST /api/processes/delete - Delete a process from the system
    server.setRoute("POST", "/api/processes/delete", [service](const HttpRequest& req) {
        try {
            JsonValue body = JsonValue::parse(req.body);
            if (!body.isObject()) {
                return HttpResponse::BadRequest("Malformed JSON request body.");
            }
            int pId = -1;
            std::string pName;
            if (body.has("processId")) {
                pId = body.get("processId").asInt(-1);
            } else if (body.has("id")) {
                if (body.get("id").isString()) {
                    std::string s = body.get("id").asString();
                    pName = s;
                    if (!s.empty() && (s[0] == 'P' || s[0] == 'p')) {
                        try { pId = std::stoi(s.substr(1)); } catch (...) { pId = -1; }
                    } else {
                        try { pId = std::stoi(s); } catch (...) { pId = -1; }
                    }
                } else {
                    pId = body.get("id").asInt(-1);
                }
            } else if (body.has("name")) {
                pName = body.get("name").asString();
            }

            // Lookup by name if pId is not parsed as number
            if (pId < 0 && !pName.empty()) {
                JsonValue curState = service->getState();
                if (curState.has("processes") && curState.get("processes").isArray()) {
                    auto procs = curState.get("processes").asArray();
                    for (size_t i = 0; i < procs.size(); ++i) {
                        if (procs[i].get("name").asString() == pName || procs[i].get("id").asString() == pName) {
                            pId = procs[i].get("rawId").asInt(static_cast<int>(i));
                            break;
                        }
                    }
                }
            }

            if (pId < 0) {
                return HttpResponse::Unprocessable("Invalid or missing process identifier.");
            }

            std::string err;
            JsonValue state = service->deleteProcess(pId, err);
            if (!err.empty()) {
                return HttpResponse::Unprocessable(err);
            }
            return HttpResponse::Ok(state.serialize());
        } catch (const std::exception& ex) {
            return HttpResponse::BadRequest(std::string("Invalid JSON: ") + ex.what());
        }
    });

    // 10c2. POST /api/processes/clear - Clear all processes
    server.setRoute("POST", "/api/processes/clear", [service](const HttpRequest&) {
        std::string err;
        JsonValue state = service->clearProcesses(err);
        return HttpResponse::Ok(state.serialize());
    });

    // 10d. POST /api/resources/delete - Delete a resource class from the system
    server.setRoute("POST", "/api/resources/delete", [service](const HttpRequest& req) {
        try {
            JsonValue body = JsonValue::parse(req.body);
            if (!body.isObject()) {
                return HttpResponse::BadRequest("Malformed JSON request body.");
            }
            int rId = -1;
            std::string rName;
            if (body.has("resourceId")) {
                rId = body.get("resourceId").asInt(-1);
            } else if (body.has("id")) {
                if (body.get("id").isString()) {
                    std::string s = body.get("id").asString();
                    rName = s;
                    if (!s.empty() && (s[0] == 'R' || s[0] == 'r')) {
                        try { rId = std::stoi(s.substr(1)); } catch (...) { rId = -1; }
                    } else {
                        try { rId = std::stoi(s); } catch (...) { rId = -1; }
                    }
                } else {
                    rId = body.get("id").asInt(-1);
                }
            } else if (body.has("name")) {
                rName = body.get("name").asString();
            }

            // Lookup by name if rId is not parsed as number
            if (rId < 0 && !rName.empty()) {
                JsonValue curState = service->getState();
                if (curState.has("resources") && curState.get("resources").isArray()) {
                    auto resList = curState.get("resources").asArray();
                    for (size_t j = 0; j < resList.size(); ++j) {
                        if (resList[j].get("name").asString() == rName || resList[j].get("id").asString() == rName) {
                            rId = resList[j].get("rawId").asInt(static_cast<int>(j));
                            break;
                        }
                    }
                }
            }

            if (rId < 0) {
                return HttpResponse::Unprocessable("Invalid or missing resource identifier.");
            }

            std::string err;
            JsonValue state = service->deleteResource(rId, err);
            if (!err.empty()) {
                return HttpResponse::Unprocessable(err);
            }
            return HttpResponse::Ok(state.serialize());
        } catch (const std::exception& ex) {
            return HttpResponse::BadRequest(std::string("Invalid JSON: ") + ex.what());
        }
    });

    // 10d2. POST /api/resources/clear - Clear all resources
    server.setRoute("POST", "/api/resources/clear", [service](const HttpRequest&) {
        std::string err;
        JsonValue state = service->clearResources(err);
        return HttpResponse::Ok(state.serialize());
    });

    // 10e. POST /api/simulation/clear - Clear all processes and resources (Blank Canvas)
    server.setRoute("POST", "/api/simulation/clear", [service](const HttpRequest&) {
        std::string err;
        JsonValue state = service->clearAll(err);
        return HttpResponse::Ok(state.serialize());
    });
}

} // namespace deadlock::bridge
