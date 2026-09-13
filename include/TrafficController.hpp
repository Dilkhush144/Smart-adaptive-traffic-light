#pragma once
#include "Lane.hpp"
#include "EmergencyVehicleDetector.hpp"
#include "Types.hpp"
#include <array>
#include <cstddef>

// ---------------------------------------------------------------------------
// TrafficController
//
// WHY THIS CLASS EXISTS:
// This is the "brain" of the intersection. It owns the scheduling
// algorithm: which direction gets green next, for how long, and how an
// emergency vehicle interrupts that schedule. Every other class (Lane,
// TrafficLight, Sensor, EmergencyVehicleDetector) is a passive building
// block; TrafficController is the one class that makes decisions.
//
// DESIGN DECISION - round-robin order + density-based duration:
// The order lanes are served in is a fixed, fair rotation
// (NORTH -> SOUTH -> EAST -> WEST -> NORTH -> ...), so no lane can be
// starved forever even if another lane is consistently busier. What
// actually adapts to traffic is the DURATION each lane gets once it is
// its turn: a busy lane gets a longer green light, a near-empty lane gets
// close to the minimum. This is simple to explain, is deterministic
// enough to unit test, and avoids the starvation problem that a purely
// "always serve the busiest lane" algorithm would have.
//
// SAFETY INVARIANT:
// Only one Lane's TrafficLight is ever GREEN at a time. This is enforced
// structurally: serveDirection() always drives every other lane to RED
// before turning the requested lane GREEN, and TrafficLight itself refuses
// illegal state jumps (see TrafficLight::setState).
// ---------------------------------------------------------------------------
class TrafficController {
public:
    // minGreenSeconds/maxGreenSeconds bound the adaptive green duration.
    // emergencyGreenSeconds is the fixed duration given to an emergency
    // lane (a rescue vehicle just needs enough time to clear the
    // intersection, so this is not density-based).
    // tickMillis controls how fast the simulation clock runs versus real
    // time, so a demo run finishes in a reasonable number of seconds.
    TrafficController(std::array<Lane, 4>& lanes,
                       EmergencyVehicleDetector& detector,
                       int minGreenSeconds = 5,
                       int maxGreenSeconds = 20,
                       int emergencyGreenSeconds = 8,
                       int tickMillis = 200);

    // Runs `cycles` full rounds of the round-robin schedule (a "cycle" =
    // NORTH, SOUTH, EAST, WEST each served once), printing state to the
    // console as it goes. Emergency requests are checked every tick and
    // can interrupt a normal green phase at any point.
    void runSimulation(int cycles);

    // --- Pure, side-effect-limited pieces of the algorithm, exposed so
    // --- they can be unit tested directly without spinning up threads or
    // --- running the full console simulation. ---

    // Adaptive green-time formula: linearly scales between
    // minGreenSeconds_ and maxGreenSeconds_ based on lane density (0..1).
    int calculateGreenDuration(double density) const;

    // Round-robin selection: returns the next direction after `fromIndex`
    // in the fixed NORTH/SOUTH/EAST/WEST order.
    static Direction nextInRotation(int fromIndex);

private:
    std::array<Lane, 4>& lanes_;
    EmergencyVehicleDetector& detector_;

    int minGreenSeconds_;
    int maxGreenSeconds_;
    int emergencyGreenSeconds_;
    int tickMillis_;

    int rotationIndex_; // index into the fixed NORTH/SOUTH/EAST/WEST order

    Lane& laneFor(Direction direction);

    // Forces every lane except `except` to RED (via YELLOW first if it
    // was GREEN). Guarantees the "no two greens at once" safety rule.
    void setAllRedExcept(Direction except);

    // Runs one normal, density-adaptive green phase for `direction`.
    // Ticks once per tickMillis_ so an emergency request can interrupt
    // mid-phase; returns true if it completed normally, false if an
    // emergency interrupted it.
    bool serveDirectionAdaptive(Direction direction);

    // Runs one fixed-duration emergency green phase for `direction`.
    void serveDirectionEmergency(Direction direction);

    // Drains and serves every currently-queued emergency request (handles
    // the "multiple emergency requests at once" case) before returning.
    void drainEmergencyQueue();
};
