// ---------------------------------------------------------------------------
// main.cpp
//
// Wires all the classes together and runs the console simulation.
//
// THREADING MODEL (kept deliberately simple):
//   - One background "sensor thread" per lane. Each thread just calls
//     Sensor::readVehicleCount() every ~300-600ms and writes the result
//     into that lane's Lane::updateVehicleCount(). This models a real
//     sensor pushing fresh readings independently of what the controller
//     is doing.
//   - One background "emergency injector" thread, used only for this demo,
//     that randomly reports an emergency vehicle on a random lane a couple
//     of times during the run - this is what you would replace with a real
//     detection mechanism (see EmergencyVehicleDetector.hpp).
//   - The MAIN thread runs TrafficController::runSimulation(), which reads
//     the shared Lane state (through its mutex-protected accessors) and
//     drives the actual traffic lights.
//
// All shared, mutable data (vehicle counts, emergency flags) lives inside
// Lane, which is responsible for protecting it - main.cpp itself never
// touches a mutex directly.
// ---------------------------------------------------------------------------
#include "Types.hpp"
#include "Sensor.hpp"
#include "Lane.hpp"
#include "EmergencyVehicleDetector.hpp"
#include "TrafficController.hpp"

#include <array>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include <iostream>
#include <cstdlib>

namespace {

void sensorLoop(Sensor& sensor, Lane& lane, std::atomic<bool>& running) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> sleepMs(300, 600);
    while (running.load()) {
        int count = sensor.readVehicleCount();
        lane.updateVehicleCount(count);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs(rng)));
    }
}

void emergencyInjectorLoop(EmergencyVehicleDetector& detector,
                            std::atomic<bool>& running) {
    static const Direction directions[] = {
        Direction::NORTH, Direction::SOUTH, Direction::EAST, Direction::WEST
    };
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> delayMs(1500, 3500);
    std::uniform_int_distribution<int> dirPick(0, 3);

    // Fire two demo emergencies during the run, spaced apart, so the
    // console output shows the interrupt-and-resume behaviour clearly.
    for (int i = 0; i < 2 && running.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs(rng)));
        if (!running.load()) break;
        Direction d = directions[dirPick(rng)];
        detector.reportEmergency(d);
    }
}

} // namespace

int main(int argc, char** argv) {
    int cycles = 2;
    if (argc > 1) {
        cycles = std::atoi(argv[1]);
        if (cycles <= 0) cycles = 2;
    }

    constexpr int laneCapacity = 20;

    std::array<Lane, 4> lanes{
        Lane(Direction::NORTH, laneCapacity),
        Lane(Direction::SOUTH, laneCapacity),
        Lane(Direction::EAST,  laneCapacity),
        Lane(Direction::WEST,  laneCapacity)
    };

    std::vector<Sensor> sensors;
    sensors.reserve(4);
    for (Lane& lane : lanes) {
        sensors.emplace_back(lane.getDirection(), laneCapacity);
    }

    std::vector<Lane*> lanePtrs;
    for (Lane& lane : lanes) lanePtrs.push_back(&lane);
    EmergencyVehicleDetector detector(lanePtrs);

    std::atomic<bool> running(true);

    std::vector<std::thread> sensorThreads;
    for (std::size_t i = 0; i < lanes.size(); ++i) {
        sensorThreads.emplace_back(sensorLoop,
                                    std::ref(sensors[i]),
                                    std::ref(lanes[i]),
                                    std::ref(running));
    }
    std::thread emergencyThread(emergencyInjectorLoop, std::ref(detector),
                                 std::ref(running));

    std::cout << "Smart Adaptive Traffic Light Controller\n";
    std::cout << "Running " << cycles << " cycle(s). "
              << "Each cycle serves NORTH, SOUTH, EAST, WEST once.\n";

    TrafficController controller(lanes, detector,
                                  /*minGreenSeconds=*/5,
                                  /*maxGreenSeconds=*/20,
                                  /*emergencyGreenSeconds=*/6,
                                  /*tickMillis=*/150);
    controller.runSimulation(cycles);

    running.store(false);
    for (auto& t : sensorThreads) t.join();
    emergencyThread.join();

    return 0;
}
