#include "TrafficLight.hpp"
#include <stdexcept>

TrafficLight::TrafficLight(Direction direction)
    : direction_(direction), state_(LightState::RED) {
    // All lights start RED. This is the only safe state to assume for
    // every approach when the system boots up.
}

bool TrafficLight::isValidTransition(LightState from, LightState to) {
    // The only legal cycle is RED -> GREEN -> YELLOW -> RED.
    // Anything else (e.g. RED -> YELLOW, GREEN -> RED) is rejected because
    // it would either confuse drivers or remove the safety buffer that
    // YELLOW provides before a light turns RED.
    if (from == to) return true; // re-asserting the same state is harmless
    switch (from) {
        case LightState::RED:    return to == LightState::GREEN;
        case LightState::GREEN:  return to == LightState::YELLOW;
        case LightState::YELLOW: return to == LightState::RED;
    }
    return false;
}

void TrafficLight::setState(LightState newState) {
    if (!isValidTransition(state_, newState)) {
        throw std::logic_error(
            "Illegal light transition for " + toString(direction_) + ": " +
            toString(state_) + " -> " + toString(newState));
    }
    state_ = newState;
}

LightState TrafficLight::getState() const {
    return state_;
}

Direction TrafficLight::getDirection() const {
    return direction_;
}
