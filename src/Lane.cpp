#include "Lane.hpp"

Lane::Lane(Direction direction, int capacity)
    : direction_(direction),
      capacity_(capacity),
      vehicleCount_(0),
      emergencyActive_(false),
      trafficLight_(direction) {}

void Lane::updateVehicleCount(int count) {
    // Defensive clamp: a real sensor (or a flaky simulated one) could
    // report a negative count or a count far above physical lane
    // capacity. Clamping here means every other class can trust that
    // getVehicleCount() is always in [0, capacity_].
    if (count < 0) count = 0;
    if (count > capacity_) count = capacity_;

    std::lock_guard<std::mutex> lock(countMutex_);
    vehicleCount_ = count;
}

int Lane::getVehicleCount() const {
    std::lock_guard<std::mutex> lock(countMutex_);
    return vehicleCount_;
}

double Lane::getDensity() const {
    if (capacity_ <= 0) return 0.0;
    int count = getVehicleCount();
    double density = static_cast<double>(count) / static_cast<double>(capacity_);
    // Defensive clamp: protects against an out-of-range/invalid sensor
    // reading ever producing a density outside [0, 1].
    if (density < 0.0) density = 0.0;
    if (density > 1.0) density = 1.0;
    return density;
}

void Lane::setEmergency(bool active) {
    emergencyActive_.store(active);
}

bool Lane::hasEmergency() const {
    return emergencyActive_.load();
}

Direction Lane::getDirection() const {
    return direction_;
}

TrafficLight& Lane::light() {
    return trafficLight_;
}

const TrafficLight& Lane::light() const {
    return trafficLight_;
}

int Lane::getCapacity() const {
    return capacity_;
}
