#pragma once
#include "Types.hpp"
#include "TrafficLight.hpp"
#include <mutex>
#include <atomic>

// ---------------------------------------------------------------------------
// Lane
//
// WHY THIS CLASS EXISTS:
// A Lane groups together everything that belongs to one approach of the
// intersection: its own TrafficLight, its current vehicle count, and
// whether an emergency vehicle is currently present in it. Without this
// class, TrafficController would need four parallel arrays (counts,
// lights, emergency flags...) all indexed by direction, which is exactly
// the kind of "primitive obsession" that gets confusing and bug-prone.
//
// THREAD SAFETY:
// The vehicle count is written by a background "sensor thread" (one per
// lane, see main.cpp) and read by the controller thread. Both happen
// concurrently, so access is protected by a mutex. The emergency flag is a
// std::atomic<bool> because it is a single word that only ever needs a
// simple set/get, so a full mutex would be unnecessary overhead for it.
// ---------------------------------------------------------------------------
class Lane {
public:
    explicit Lane(Direction direction, int capacity = 20);

    // Thread-safe: called by the sensor thread whenever a new reading
    // comes in.
    void updateVehicleCount(int count);

    // Thread-safe: called by the controller thread to read the latest
    // known count.
    int getVehicleCount() const;

    // Density is just vehicleCount / capacity, expressed as 0.0 - 1.0.
    // Kept as a method (not stored) so it's always derived from the latest
    // count and can never go stale.
    double getDensity() const;

    void setEmergency(bool active);
    bool hasEmergency() const;

    Direction getDirection() const;

    TrafficLight& light();
    const TrafficLight& light() const;

    int getCapacity() const;

private:
    Direction direction_;
    int capacity_;

    mutable std::mutex countMutex_;
    int vehicleCount_;

    std::atomic<bool> emergencyActive_;

    TrafficLight trafficLight_;
};
