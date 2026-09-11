#include "Sensor.hpp"
#include <chrono>

Sensor::Sensor(Direction direction, int laneCapacity)
    : direction_(direction),
      laneCapacity_(laneCapacity),
      // Seed each sensor's RNG independently (based on the clock + a small
      // per-direction offset) so the four lanes don't all produce the exact
      // same sequence of "random" readings.
      rng_(static_cast<unsigned>(
          std::chrono::steady_clock::now().time_since_epoch().count()) +
          static_cast<unsigned>(direction) * 7919u),
      distribution_(0, laneCapacity) {}

int Sensor::readVehicleCount() {
    return distribution_(rng_);
}

Direction Sensor::getDirection() const {
    return direction_;
}
