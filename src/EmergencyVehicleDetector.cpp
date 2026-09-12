#include "EmergencyVehicleDetector.hpp"
#include <stdexcept>

EmergencyVehicleDetector::EmergencyVehicleDetector(std::vector<Lane*> lanes)
    : lanes_(std::move(lanes)) {}

Lane* EmergencyVehicleDetector::findLane(Direction direction) const {
    for (Lane* lane : lanes_) {
        if (lane->getDirection() == direction) return lane;
    }
    throw std::invalid_argument("EmergencyVehicleDetector: unknown direction");
}

void EmergencyVehicleDetector::reportEmergency(Direction direction) {
    findLane(direction)->setEmergency(true);
}

void EmergencyVehicleDetector::clearEmergency(Direction direction) {
    findLane(direction)->setEmergency(false);
}

std::optional<Direction> EmergencyVehicleDetector::checkForEmergency() const {
    // Fixed scan order keeps behaviour deterministic and easy to explain
    // (and to test) when multiple lanes report an emergency simultaneously.
    static const Direction order[] = {
        Direction::NORTH, Direction::SOUTH, Direction::EAST, Direction::WEST
    };
    for (Direction d : order) {
        Lane* lane = findLane(d);
        if (lane->hasEmergency()) return d;
    }
    return std::nullopt;
}
