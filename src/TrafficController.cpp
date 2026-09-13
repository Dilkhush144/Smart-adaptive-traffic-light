#include "TrafficController.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <stdexcept>

namespace {
constexpr Direction kRotationOrder[4] = {
    Direction::NORTH, Direction::SOUTH, Direction::EAST, Direction::WEST
};
}

TrafficController::TrafficController(std::array<Lane, 4>& lanes,
                                     EmergencyVehicleDetector& detector,
                                     int minGreenSeconds,
                                     int maxGreenSeconds,
                                     int emergencyGreenSeconds,
                                     int tickMillis)
    : lanes_(lanes),
      detector_(detector),
      minGreenSeconds_(minGreenSeconds),
      maxGreenSeconds_(maxGreenSeconds),
      emergencyGreenSeconds_(emergencyGreenSeconds),
      tickMillis_(tickMillis),
      rotationIndex_(0) {
    if (minGreenSeconds_ <= 0 || maxGreenSeconds_ < minGreenSeconds_) {
        throw std::invalid_argument(
            "TrafficController: invalid min/max green duration configuration");
    }
}

Direction TrafficController::nextInRotation(int fromIndex) {
    int idx = ((fromIndex % 4) + 4) % 4; // defensive: works even if negative
    return kRotationOrder[idx];
}

Lane& TrafficController::laneFor(Direction direction) {
    for (Lane& lane : lanes_) {
        if (lane.getDirection() == direction) return lane;
    }
    throw std::invalid_argument("TrafficController: unknown direction");
}

int TrafficController::calculateGreenDuration(double density) const {
    // Defensive clamp against an invalid/extreme sensor value slipping
    // through (Lane::getDensity() already clamps to [0,1], this is a
    // second line of defence so the formula below is never fed garbage).
    if (density < 0.0) density = 0.0;
    if (density > 1.0) density = 1.0;

    double span = static_cast<double>(maxGreenSeconds_ - minGreenSeconds_);
    int duration = minGreenSeconds_ + static_cast<int>(density * span + 0.5);
    return duration;
}

void TrafficController::setAllRedExcept(Direction except) {
    for (Lane& lane : lanes_) {
        if (lane.getDirection() == except) continue;
        if (lane.light().getState() == LightState::GREEN) {
            lane.light().setState(LightState::YELLOW);
            lane.light().setState(LightState::RED);
        } else if (lane.light().getState() == LightState::YELLOW) {
            lane.light().setState(LightState::RED);
        }
        // If already RED, nothing to do.
    }
}

bool TrafficController::serveDirectionAdaptive(Direction direction) {
    Lane& lane = laneFor(direction);

    // Safety rule: everyone else must be RED before this lane goes GREEN.
    setAllRedExcept(direction);

    double density = lane.getDensity();
    int durationSeconds = calculateGreenDuration(density);

    std::cout << "\n[NORMAL] " << toString(direction)
              << " | vehicles=" << lane.getVehicleCount()
              << " | density=" << std::fixed << std::setprecision(2) << density
              << " | green=" << durationSeconds << "s\n";

    lane.light().setState(LightState::GREEN);

    for (int elapsed = 0; elapsed < durationSeconds; ++elapsed) {
        std::this_thread::sleep_for(std::chrono::milliseconds(tickMillis_));
        if (detector_.checkForEmergency().has_value()) {
            std::cout << "  >> Emergency vehicle detected mid-phase, "
                      << "interrupting " << toString(direction) << "\n";
            lane.light().setState(LightState::YELLOW);
            lane.light().setState(LightState::RED);
            return false;
        }
    }

    lane.light().setState(LightState::YELLOW);
    std::this_thread::sleep_for(std::chrono::milliseconds(tickMillis_));
    lane.light().setState(LightState::RED);
    return true;
}

void TrafficController::serveDirectionEmergency(Direction direction) {
    Lane& lane = laneFor(direction);

    setAllRedExcept(direction);

    std::cout << "\n[EMERGENCY] " << toString(direction)
              << " | clearing for " << emergencyGreenSeconds_ << "s\n";

    lane.light().setState(LightState::GREEN);
    for (int elapsed = 0; elapsed < emergencyGreenSeconds_; ++elapsed) {
        std::this_thread::sleep_for(std::chrono::milliseconds(tickMillis_));
    }
    lane.light().setState(LightState::YELLOW);
    std::this_thread::sleep_for(std::chrono::milliseconds(tickMillis_));
    lane.light().setState(LightState::RED);

    detector_.clearEmergency(direction);
    std::cout << "  << " << toString(direction)
              << " emergency vehicle has cleared the intersection\n";
}

void TrafficController::drainEmergencyQueue() {
    // Loops because more than one lane can report an emergency at once
    // (the "multiple emergency requests" test case). Each call to
    // checkForEmergency() returns the highest-priority still-pending
    // lane (fixed NORTH/SOUTH/EAST/WEST order), so this naturally serves
    // them one at a time until none remain.
    while (auto emergencyDir = detector_.checkForEmergency()) {
        serveDirectionEmergency(*emergencyDir);
    }
}

void TrafficController::runSimulation(int cycles) {
    if (cycles <= 0) {
        throw std::invalid_argument("runSimulation: cycles must be positive");
    }

    for (int cycle = 0; cycle < cycles; ++cycle) {
        std::cout << "\n===== Cycle " << (cycle + 1) << "/" << cycles
                  << " =====\n";

        for (int step = 0; step < 4; ++step) {
            drainEmergencyQueue(); // handle anyone waiting before we pick a lane

            Direction direction = nextInRotation(rotationIndex_);
            rotationIndex_ = (rotationIndex_ + 1) % 4;

            bool completedNormally = serveDirectionAdaptive(direction);
            if (!completedNormally) {
                drainEmergencyQueue(); // serve whoever interrupted us
            }
        }
    }

    std::cout << "\nSimulation finished. All lanes RED (safe idle state).\n";
}
