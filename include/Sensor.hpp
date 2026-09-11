#pragma once
#include "Types.hpp"
#include <random>

// ---------------------------------------------------------------------------
// Sensor
//
// WHY THIS CLASS EXISTS:
// In the real world this would wrap an inductive loop sensor, an
// ultrasonic sensor, or a camera-based counter mounted on a lane. In this
// project it is SIMULATED: readVehicleCount() generates a plausible vehicle
// count instead of reading real hardware.
//
// This is deliberate ABSTRACTION: everything above this class (Lane,
// TrafficController) only calls readVehicleCount() and does not know or
// care whether the number came from a simulation or a real sensor. That
// means the simulated Sensor could be swapped for a real hardware driver
// later without changing any other class. See hardware/README.md.
// ---------------------------------------------------------------------------
class Sensor {
public:
    explicit Sensor(Direction direction, int laneCapacity = 20);

    // Returns a simulated vehicle count currently waiting in the lane,
    // in the range [0, laneCapacity]. Each call represents one "reading".
    int readVehicleCount();

    Direction getDirection() const;

private:
    Direction direction_;
    int laneCapacity_;
    std::mt19937 rng_;
    std::uniform_int_distribution<int> distribution_;
};
