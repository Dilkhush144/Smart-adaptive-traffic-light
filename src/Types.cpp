#include "Types.hpp"

std::string toString(Direction d) {
    switch (d) {
        case Direction::NORTH: return "NORTH";
        case Direction::SOUTH: return "SOUTH";
        case Direction::EAST:  return "EAST";
        case Direction::WEST:  return "WEST";
    }
    return "UNKNOWN";
}

std::string toString(LightState s) {
    switch (s) {
        case LightState::RED:    return "RED";
        case LightState::YELLOW: return "YELLOW";
        case LightState::GREEN:  return "GREEN";
    }
    return "UNKNOWN";
}
