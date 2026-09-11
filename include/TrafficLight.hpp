#pragma once
#include "Types.hpp"

// ---------------------------------------------------------------------------
// TrafficLight
//
// WHY THIS CLASS EXISTS:
// A traffic light is a real physical device with exactly one job: show a
// state (RED/YELLOW/GREEN) and refuse to skip states in an unsafe way.
// Pulling this out as its own class means the "is this transition legal?"
// rule lives in exactly one place, instead of being re-checked (or
// forgotten) everywhere the controller changes a light.
//
// This is a small example of ENCAPSULATION: the current state is private,
// and the only way to change it is through setState(), which enforces the
// safety rule below.
// ---------------------------------------------------------------------------
class TrafficLight {
public:
    explicit TrafficLight(Direction direction);

    // Attempts to move to newState. Throws std::logic_error if the
    // transition is not part of the standard RED -> GREEN -> YELLOW -> RED
    // cycle. This is what guarantees a light can never "jump" straight
    // from RED to YELLOW, or stay GREEN forever without cycling through
    // YELLOW first.
    void setState(LightState newState);

    LightState getState() const;
    Direction getDirection() const;

private:
    Direction direction_;
    LightState state_;

    static bool isValidTransition(LightState from, LightState to);
};
