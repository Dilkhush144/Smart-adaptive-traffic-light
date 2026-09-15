# System Architecture

## Class overview

| Class | Responsibility | Why it's separate |
|---|---|---|
| `TrafficLight` | Holds one lane's RED/YELLOW/GREEN state and rejects illegal transitions. | Keeps the "what transitions are safe" rule in exactly one place. |
| `Sensor` | Simulates a vehicle-count reading for one lane. | Abstracts *how* a count is obtained, so a real sensor driver could replace it later without touching anything else. |
| `Lane` | Groups a direction's `TrafficLight`, current vehicle count, and emergency flag; guards shared state with a mutex. | Avoids four parallel arrays indexed by direction; centralizes thread-safety for that lane's data. |
| `EmergencyVehicleDetector` | Tracks which lanes currently have an emergency vehicle waiting, in a fixed, deterministic priority order. | Keeps emergency-specific logic out of the scheduler so both stay easy to read and test. |
| `TrafficController` | Owns the scheduling algorithm: round-robin rotation, density-based green duration, and emergency interruption. | The one class that makes decisions; everything else is a passive data holder. |

## Data flow

See [`docs/diagrams/adaptive_timing_flow.md`](diagrams/adaptive_timing_flow.md)
and [`docs/diagrams/emergency_priority_flow.md`](diagrams/emergency_priority_flow.md).

In short:

1. Four background **sensor threads** (one per lane, started in `main.cpp`)
   continuously call `Sensor::readVehicleCount()` and push results into
   their `Lane` via `Lane::updateVehicleCount()`.
2. The **main thread** runs `TrafficController::runSimulation()`, which
   loops through directions in a fixed, fair rotation
   (NORTH → SOUTH → EAST → WEST → …).
3. For each direction served, the controller reads that lane's current
   density (`Lane::getDensity()`) and computes a green duration with a
   simple linear formula.
4. While a green phase is running, the controller checks
   `EmergencyVehicleDetector::checkForEmergency()` every tick. If an
   emergency is reported, the current phase is safely cut short and the
   emergency lane is served immediately, before normal rotation resumes.

## Safety invariant

Only one `TrafficLight` is ever `GREEN` at a time. This is enforced two
ways:

- `TrafficController::setAllRedExcept()` always forces every other lane to
  `RED` (via `YELLOW` first if it was `GREEN`) *before* the target lane is
  set to `GREEN`.
- `TrafficLight::setState()` independently rejects any transition that
  isn't part of the `RED → GREEN → YELLOW → RED` cycle, so even a bug in
  the controller couldn't skip a light straight to an unsafe state.

`tests/test_main.cpp` includes an integration test that runs the real
simulation on background threads while a separate watcher thread samples
all four lights every millisecond and asserts at most one is ever `GREEN`.

## Concurrency model

| Thread | Role | Shared data it touches |
|---|---|---|
| 4x sensor threads | Simulate periodic vehicle-count readings | `Lane::vehicleCount_` (via mutex) |
| 1x emergency injector thread (demo only) | Randomly reports emergencies | `Lane::emergencyActive_` (via `std::atomic<bool>`) |
| main thread | Runs the controller loop, drives the lights | Reads the above; owns all `TrafficLight` state itself (never touched by another thread) |

`vehicleCount_` needs a `std::mutex` because it's read and written from
different threads and a torn/partial read would be a real bug.
`emergencyActive_` is a single boolean, so a `std::atomic<bool>` is enough
and avoids locking overhead for such a simple flag.
