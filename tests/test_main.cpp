// ---------------------------------------------------------------------------
// test_main.cpp
//
// A deliberately small, dependency-free test runner (no GoogleTest/Catch2)
// so the whole test suite compiles with nothing more than a C++17
// compiler - matching the "keep it simple" spirit of the rest of the
// project. Each test function returns void and calls CHECK(); failures
// are collected and reported at the end, and the process exits non-zero
// if anything failed (so it plugs into CI easily later).
// ---------------------------------------------------------------------------
#include "Types.hpp"
#include "TrafficLight.hpp"
#include "Sensor.hpp"
#include "Lane.hpp"
#include "EmergencyVehicleDetector.hpp"
#include "TrafficController.hpp"

#include <iostream>
#include <array>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <stdexcept>

namespace {

int g_checks = 0;
int g_failures = 0;

void reportCheck(bool condition, const std::string& description,
                  const char* file, int line) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cerr << "  [FAIL] " << description << " (" << file << ":" << line
                  << ")\n";
    }
}

#define CHECK(cond) reportCheck((cond), #cond, __FILE__, __LINE__)

void runTest(const std::string& name, void (*testFn)()) {
    std::cout << "Running: " << name << "\n";
    int before = g_failures;
    testFn();
    if (g_failures == before) {
        std::cout << "  [PASS]\n";
    }
}

// ---------------------------------------------------------------------------
// TrafficLight: safe transitions only
// ---------------------------------------------------------------------------
void test_TrafficLight_validCycleWorks() {
    TrafficLight light(Direction::NORTH);
    CHECK(light.getState() == LightState::RED);
    light.setState(LightState::GREEN);
    CHECK(light.getState() == LightState::GREEN);
    light.setState(LightState::YELLOW);
    CHECK(light.getState() == LightState::YELLOW);
    light.setState(LightState::RED);
    CHECK(light.getState() == LightState::RED);
}

void test_TrafficLight_illegalTransitionThrows() {
    TrafficLight light(Direction::EAST);
    bool threw = false;
    try {
        light.setState(LightState::YELLOW); // RED -> YELLOW is not allowed
    } catch (const std::logic_error&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(light.getState() == LightState::RED); // state unchanged after failed attempt
}

// ---------------------------------------------------------------------------
// Lane: density calculation + defensive clamping of bad sensor values
// ---------------------------------------------------------------------------
void test_Lane_densityCalculation_lowTraffic() {
    Lane lane(Direction::NORTH, 20);
    lane.updateVehicleCount(2); // low traffic
    CHECK(lane.getVehicleCount() == 2);
    CHECK(lane.getDensity() > 0.05 && lane.getDensity() < 0.15);
}

void test_Lane_densityCalculation_highTraffic() {
    Lane lane(Direction::SOUTH, 20);
    lane.updateVehicleCount(18); // high traffic
    CHECK(lane.getDensity() > 0.85 && lane.getDensity() <= 1.0);
}

void test_Lane_invalidExtremeSensorValues_areClamped() {
    Lane lane(Direction::EAST, 20);
    lane.updateVehicleCount(-50); // invalid negative reading
    CHECK(lane.getVehicleCount() == 0);
    CHECK(lane.getDensity() == 0.0);

    lane.updateVehicleCount(999999); // absurd, far above physical capacity
    CHECK(lane.getVehicleCount() == 20);
    CHECK(lane.getDensity() == 1.0);
}

// ---------------------------------------------------------------------------
// EmergencyVehicleDetector: detection, clearing, and multiple requests
// ---------------------------------------------------------------------------
void test_EmergencyDetector_reportAndClear() {
    Lane north(Direction::NORTH, 20), south(Direction::SOUTH, 20),
         east(Direction::EAST, 20), west(Direction::WEST, 20);
    EmergencyVehicleDetector detector({&north, &south, &east, &west});

    CHECK(!detector.checkForEmergency().has_value());

    detector.reportEmergency(Direction::SOUTH);
    auto found = detector.checkForEmergency();
    CHECK(found.has_value() && *found == Direction::SOUTH);

    detector.clearEmergency(Direction::SOUTH);
    CHECK(!detector.checkForEmergency().has_value());
}

void test_EmergencyDetector_multipleRequests_fixedPriorityOrder() {
    Lane north(Direction::NORTH, 20), south(Direction::SOUTH, 20),
         east(Direction::EAST, 20), west(Direction::WEST, 20);
    EmergencyVehicleDetector detector({&north, &south, &east, &west});

    // Report WEST first, then EAST - fixed scan order should still
    // surface EAST first because it comes before WEST in N/S/E/W order.
    detector.reportEmergency(Direction::WEST);
    detector.reportEmergency(Direction::EAST);

    auto first = detector.checkForEmergency();
    CHECK(first.has_value() && *first == Direction::EAST);

    detector.clearEmergency(Direction::EAST);
    auto second = detector.checkForEmergency();
    CHECK(second.has_value() && *second == Direction::WEST);

    detector.clearEmergency(Direction::WEST);
    CHECK(!detector.checkForEmergency().has_value());
}

// ---------------------------------------------------------------------------
// TrafficController: green-time formula, rotation fairness, config validation
// ---------------------------------------------------------------------------
void test_Controller_greenDuration_scalesWithDensity() {
    std::array<Lane, 4> lanes{
        Lane(Direction::NORTH, 20), Lane(Direction::SOUTH, 20),
        Lane(Direction::EAST, 20),  Lane(Direction::WEST, 20)
    };
    std::vector<Lane*> ptrs{&lanes[0], &lanes[1], &lanes[2], &lanes[3]};
    EmergencyVehicleDetector detector(ptrs);
    TrafficController controller(lanes, detector, /*min=*/5, /*max=*/20,
                                  /*emergencyGreen=*/8, /*tickMillis=*/10);

    CHECK(controller.calculateGreenDuration(0.0) == 5);   // empty lane -> minimum
    CHECK(controller.calculateGreenDuration(1.0) == 20);  // full lane -> maximum
    int mid = controller.calculateGreenDuration(0.5);
    CHECK(mid >= 12 && mid <= 13);                        // roughly halfway
}

void test_Controller_invalidConfig_throws() {
    std::array<Lane, 4> lanes{
        Lane(Direction::NORTH, 20), Lane(Direction::SOUTH, 20),
        Lane(Direction::EAST, 20),  Lane(Direction::WEST, 20)
    };
    std::vector<Lane*> ptrs{&lanes[0], &lanes[1], &lanes[2], &lanes[3]};
    EmergencyVehicleDetector detector(ptrs);

    bool threw = false;
    try {
        TrafficController bad(lanes, detector, /*min=*/0, /*max=*/20);
        (void)bad;
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

void test_Controller_rotationIsFairRoundRobin() {
    CHECK(TrafficController::nextInRotation(0) == Direction::NORTH);
    CHECK(TrafficController::nextInRotation(1) == Direction::SOUTH);
    CHECK(TrafficController::nextInRotation(2) == Direction::EAST);
    CHECK(TrafficController::nextInRotation(3) == Direction::WEST);
    CHECK(TrafficController::nextInRotation(4) == Direction::NORTH); // wraps
}

// ---------------------------------------------------------------------------
// Integration test: run a real (fast, tickMillis=5) simulation while an
// emergency is injected mid-run from another thread. This exercises:
//   - "emergency vehicle appearing while another lane has green"
//   - "multiple emergency requests" (two are fired)
//   - the safety invariant that conflicting directions are never GREEN
//     at the same time, sampled by a watcher thread throughout the run.
// ---------------------------------------------------------------------------
void test_Integration_emergencyInterrupt_neverTwoGreensAtOnce() {
    std::array<Lane, 4> lanes{
        Lane(Direction::NORTH, 20), Lane(Direction::SOUTH, 20),
        Lane(Direction::EAST, 20),  Lane(Direction::WEST, 20)
    };
    for (Lane& lane : lanes) lane.updateVehicleCount(10);

    std::vector<Lane*> ptrs{&lanes[0], &lanes[1], &lanes[2], &lanes[3]};
    EmergencyVehicleDetector detector(ptrs);
    TrafficController controller(lanes, detector, /*min=*/1, /*max=*/2,
                                  /*emergencyGreen=*/1, /*tickMillis=*/5);

    std::atomic<bool> keepWatching(true);
    std::atomic<bool> sawConflict(false);

    std::thread watcher([&]() {
        while (keepWatching.load()) {
            int greenCount = 0;
            for (Lane& lane : lanes) {
                if (lane.light().getState() == LightState::GREEN) ++greenCount;
            }
            if (greenCount > 1) sawConflict.store(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    std::thread emergencyThread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        detector.reportEmergency(Direction::WEST);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        detector.reportEmergency(Direction::EAST); // second, concurrent request
    });

    controller.runSimulation(1);

    emergencyThread.join();
    keepWatching.store(false);
    watcher.join();

    CHECK(!sawConflict.load());
    for (Lane& lane : lanes) {
        CHECK(lane.light().getState() == LightState::RED); // ends safely idle
        CHECK(!lane.hasEmergency());                        // fully cleared
    }
}

} // namespace

int main() {
    runTest("TrafficLight_validCycleWorks", test_TrafficLight_validCycleWorks);
    runTest("TrafficLight_illegalTransitionThrows",
            test_TrafficLight_illegalTransitionThrows);
    runTest("Lane_densityCalculation_lowTraffic",
            test_Lane_densityCalculation_lowTraffic);
    runTest("Lane_densityCalculation_highTraffic",
            test_Lane_densityCalculation_highTraffic);
    runTest("Lane_invalidExtremeSensorValues_areClamped",
            test_Lane_invalidExtremeSensorValues_areClamped);
    runTest("EmergencyDetector_reportAndClear",
            test_EmergencyDetector_reportAndClear);
    runTest("EmergencyDetector_multipleRequests_fixedPriorityOrder",
            test_EmergencyDetector_multipleRequests_fixedPriorityOrder);
    runTest("Controller_greenDuration_scalesWithDensity",
            test_Controller_greenDuration_scalesWithDensity);
    runTest("Controller_invalidConfig_throws",
            test_Controller_invalidConfig_throws);
    runTest("Controller_rotationIsFairRoundRobin",
            test_Controller_rotationIsFairRoundRobin);
    runTest("Integration_emergencyInterrupt_neverTwoGreensAtOnce",
            test_Integration_emergencyInterrupt_neverTwoGreensAtOnce);

    std::cout << "\n" << g_checks << " checks run, " << g_failures
              << " failed.\n";
    return g_failures == 0 ? 0 : 1;
}
