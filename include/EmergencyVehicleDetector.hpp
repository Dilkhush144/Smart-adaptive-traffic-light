#pragma once
#include "Lane.hpp"
#include "Types.hpp"
#include <vector>
#include <optional>

// ---------------------------------------------------------------------------
// EmergencyVehicleDetector
//
// WHY THIS CLASS EXISTS:
// Emergency detection is a separate CONCERN from "how do I compute green
// time" or "which light is on". Splitting it into its own class keeps
// TrafficController focused on scheduling, and keeps the emergency-specific
// logic (how a vehicle is reported, how it's cleared, which lane wins if
// two report at once) in one testable place.
//
// In this simulated version, "detection" means something in the code
// (a test, or a demo thread in main.cpp) calls reportEmergency(direction).
// On real hardware this could instead be triggered by an RF/RFID
// transponder reader, a siren-detecting microphone, or a V2X message -
// the rest of the system would not need to change, because it only ever
// asks this class "is anyone waiting for priority right now?".
// ---------------------------------------------------------------------------
class EmergencyVehicleDetector {
public:
    explicit EmergencyVehicleDetector(std::vector<Lane*> lanes);

    // Marks direction as having an emergency vehicle waiting.
    void reportEmergency(Direction direction);

    // Clears the emergency flag once the vehicle has passed through.
    void clearEmergency(Direction direction);

    // Returns the direction that should be served next under emergency
    // priority, or std::nullopt if no lane currently reports one.
    //
    // Selection rule (kept intentionally simple and explainable): if
    // multiple lanes report an emergency at the same time, the lanes are
    // checked in a fixed order (NORTH, SOUTH, EAST, WEST) and the first
    // one found is served first; the others remain queued and are served
    // immediately after, before normal adaptive control resumes.
    std::optional<Direction> checkForEmergency() const;

private:
    std::vector<Lane*> lanes_;
    Lane* findLane(Direction direction) const;
};
