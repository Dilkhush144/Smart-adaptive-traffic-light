# Smart Adaptive Traffic Light Controller with Emergency Vehicle Priority

A console-based C++ simulation of a 4-way traffic intersection that
adjusts green-light timing to real-time traffic density and interrupts
normal operation to give emergency vehicles (e.g. ambulances) priority.

> **Note:** This system is implemented as a software-based
> **simulation** — vehicle counts come from a simulated `Sensor` class,
> not physical hardware, and there is no AI/ML, no cloud service, and no
> real-world deployment. See [`hardware/README.md`](hardware/README.md)
> for what a physical build would additionally require.

## Problem Statement

Fixed-timing traffic lights waste time holding a green light for an empty
lane while a busy lane sits at red, and they have no way to let an
ambulance or fire truck through faster than everyone else. This project
builds a small, understandable C++ system that fixes both problems:
green-light duration adapts to how busy a lane actually is, and any lane
reporting an emergency vehicle is served immediately and safely.

## Motivation

This was built as a C++/embedded-systems portfolio project — something
small enough to fully explain line-by-line in a technical interview, but
that still demonstrates real OOP design, thread-safety reasoning, and an
explainable algorithm.

## Objectives

- Model a 4-way intersection with independently controlled lights.
- Compute green-light duration from live traffic density instead of a
  fixed timer.
- Detect and safely prioritize an emergency vehicle in any lane.
- Guarantee conflicting directions are never green at the same time.
- Keep the whole thing runnable on a normal computer, no hardware needed.

## Features

- Density-based adaptive green-light duration (linear formula, bounded
  between a configurable minimum and maximum).
- Fair round-robin lane rotation so no direction is starved.
- Emergency vehicle priority that can interrupt a green phase **mid-way
  through**, safely transitions conflicting lights to red first, serves
  the emergency lane, then resumes normal control.
- Handles multiple simultaneous emergency requests (served one at a time,
  in a fixed, deterministic order).
- Defensive handling of invalid/extreme sensor readings (clamped to a
  valid range instead of corrupting the schedule).
- Multithreaded sensor simulation (one background thread per lane) with
  mutex-protected shared state.
- 11 automated tests, including a real multithreaded integration test
  that verifies conflicting lights are never green at the same time.

## System Architecture

See [`docs/architecture.md`](docs/architecture.md) for the full class
table and concurrency model. Short version:

```
Sensor (x4, simulated) -> Lane (count, density, light, emergency flag)
                              -> TrafficController (scheduling + safety)
                                    -> TrafficLight (x4, enforces safe transitions)

EmergencyVehicleDetector tracks per-lane emergency flags and is polled by
TrafficController every tick so it can interrupt a normal phase.
```

Diagrams:
- [Sensor → density → controller → signal timing](docs/diagrams/adaptive_timing_flow.md)
- [Emergency detection → priority mode → normal mode](docs/diagrams/emergency_priority_flow.md)

## How the System Works

1. Four `Lane` objects (NORTH, SOUTH, EAST, WEST) are created, each with
   its own `Sensor` and `TrafficLight`.
2. Four background threads continuously simulate sensor readings and
   push them into their `Lane`.
3. `TrafficController::runSimulation()` serves directions in a fixed
   round-robin order. For the direction being served, it:
   - reads that lane's current density,
   - computes a green duration from it,
   - forces every other lane to RED,
   - holds the target lane GREEN for the computed duration, checking for
     emergency interrupts every tick,
   - transitions GREEN → YELLOW → RED before moving to the next
     direction.
4. If `EmergencyVehicleDetector` reports a waiting emergency vehicle at
   any point, the controller cuts the current phase short, safely reds
   out every lane, serves the emergency lane for a fixed clearance time,
   clears the flag, and only then resumes the normal rotation (draining
   any additional queued emergencies first).

## Algorithm

**Traffic density:** `density = vehicleCount / laneCapacity`, clamped to
`[0.0, 1.0]` (see `Lane::getDensity()`).

**Green-light duration (adaptive part):**
```
green = minGreen + round(density * (maxGreen - minGreen))
```
`density = 0` → `minGreen`. `density = 1` → `maxGreen`. Everything else
scales linearly in between. See
`TrafficController::calculateGreenDuration()`.

**Next lane selection (fairness part):** a fixed round-robin rotation,
NORTH → SOUTH → EAST → WEST → NORTH → …, so density affects *how long*
a lane gets, not *whether* it gets a turn — this avoids starving a lane
that happens to be quiet for a while. See
`TrafficController::nextInRotation()`.

**Emergency priority:** any lane with its emergency flag set is served
before the next normally-scheduled lane, even interrupting an in-progress
green phase. If multiple lanes report an emergency, they are drained one
at a time in a fixed NORTH/SOUTH/EAST/WEST priority order before normal
control resumes. See `TrafficController::drainEmergencyQueue()` and
`EmergencyVehicleDetector::checkForEmergency()`.

## OOP Design

| Concept | Where it shows up |
|---|---|
| Encapsulation | `TrafficLight` keeps its state private; the only way to change it is `setState()`, which enforces the safe-transition rule. `Lane` keeps its vehicle count private behind a mutex. |
| Abstraction | `Sensor` hides whether a reading is simulated or (in a future version) real hardware behind one method, `readVehicleCount()`. |
| Constructors | Every class takes what it needs to be valid at construction time (e.g. `Lane(Direction, capacity)`, `TrafficController(lanes, detector, minGreen, maxGreen, ...)`), so there's no "half-built" object floating around. |
| Composition | `Lane` *has a* `TrafficLight`; `TrafficController` *has references to* `Lane`s and an `EmergencyVehicleDetector`. Composition (not inheritance) models these "made of" relationships, because a Lane genuinely is not a kind of TrafficLight — it owns one. |

**No inheritance or polymorphism is used.** All four directions behave
identically (same `TrafficLight` logic, same `Lane` logic) — there is no
genuine specialization to justify a class hierarchy, and forcing one in
would only add indirection without benefit. This was a deliberate choice,
not an oversight.

## Multithreading

- One background thread per lane simulates that lane's sensor pushing
  fresh readings (`sensorLoop` in `main.cpp`), independent of what the
  controller is doing.
- The main thread runs the controller loop and reads that shared state.
- `Lane::vehicleCount_` is protected by a `std::mutex` because it's a
  multi-step read/write from different threads.
- `Lane::emergencyActive_` is a `std::atomic<bool>` — a single flag with
  a simple set/get doesn't need a full mutex.
- A demo-only "emergency injector" thread randomly reports emergencies
  during a run so the console output actually shows the interrupt
  behaviour; this stands in for a real detection mechanism.

## Hardware Requirements

**For the main version: none.** It runs entirely as a console
application on a normal computer.

An optional future physical build is discussed (not implemented) in
[`hardware/README.md`](hardware/README.md).

## Software Requirements

- A C++17-capable compiler: GCC 8+/Clang 7+/MSVC 2019+ (developed and
  tested here with GCC 13).
- `make` (or run the `g++` command directly — see below).
- POSIX threads support (`-pthread`) — already available on Linux/macOS,
  and via MinGW-w64 on Windows.

## Installation / Setup

```bash
git clone https://github.com/<your-username>/Smart-Adaptive-Traffic-Light.git
cd Smart-Adaptive-Traffic-Light
```

No external dependencies to install — everything used is in the C++
standard library (`<thread>`, `<mutex>`, `<atomic>`, `<random>`, `<chrono>`).

## How to Compile and Run

### Linux / macOS

```bash
make            # builds build/traffic_controller
make run        # builds (if needed) and runs it, default 2 cycles
./build/traffic_controller 3    # or run directly, with a custom cycle count
make tests      # builds and runs the automated test suite
make clean      # removes build/
```

### Windows

Install a C++17 compiler with `-pthread` support — the simplest option is
[MSYS2](https://www.msys2.org/) with the `mingw-w64-x86_64-gcc` package
(this gives you `g++` and `make` in an MSYS2/MinGW terminal), or use
WSL and follow the Linux instructions above.

From an MSYS2 MinGW64 terminal, in the project folder:

```bash
make
./build/traffic_controller.exe
make tests
```

Or without `make`, directly:

```bash
g++ -std=c++17 -Wall -Wextra -Iinclude -pthread -o traffic_controller.exe src\*.cpp
traffic_controller.exe
```

## Example Output

A full real run (captured from an actual build, not hand-written) is in
[`examples/sample_output.txt`](examples/sample_output.txt). Excerpt:

```
Smart Adaptive Traffic Light Controller
Running 2 cycle(s). Each cycle serves NORTH, SOUTH, EAST, WEST once.

===== Cycle 1/2 =====

[NORMAL] NORTH | vehicles=6 | density=0.30 | green=10s

[NORMAL] SOUTH | vehicles=18 | density=0.90 | green=19s
  >> Emergency vehicle detected mid-phase, interrupting SOUTH

[EMERGENCY] WEST | clearing for 6s
  << WEST emergency vehicle has cleared the intersection

[NORMAL] EAST | vehicles=11 | density=0.55 | green=13s
...
Simulation finished. All lanes RED (safe idle state).
```

## Testing

Run with `make tests`. The suite (`tests/test_main.cpp`) is plain C++
with a tiny custom `CHECK()` macro — no external test framework — and
covers:

- Low traffic → near-minimum green duration.
- High traffic → near-maximum green duration.
- Unequal density across lanes producing different durations.
- Emergency vehicle detection and clearing.
- Multiple simultaneous emergency requests, served in a fixed order.
- Invalid/extreme sensor values (negative, far above capacity) being
  clamped instead of corrupting the schedule.
- Illegal light-state transitions being rejected.
- Invalid controller configuration (e.g. `maxGreen < minGreen`) being
  rejected.
- A real, threaded integration run where an emergency interrupts a lane
  that already has green, including a watcher thread that asserts at
  most one light is ever GREEN at any sampled instant.

## Future Improvements

- Weighted/priority-based lane selection instead of strict round-robin
  (e.g. serve the busiest lane next, with an anti-starvation cap).
- Pedestrian crossing signals and a "walk" phase.
- Logging results to a file for later analysis.
- A real Arduino/ESP32 hardware build (see `hardware/README.md`).
- Left-turn arrows / more than four simple phases.

## Limitations

- This is a **simulation**: vehicle counts and emergency detection are
  generated in software, not read from physical sensors.
- Only one direction is ever green at a time (no simultaneous
  non-conflicting phases like N+S together), which is simpler and safer
  to reason about but less throughput-optimal than a real traffic
  engineering system.
- The emergency priority order when multiple lanes report at once is a
  fixed NORTH/SOUTH/EAST/WEST scan, not based on which vehicle arrived
  first or its distance from the intersection.
- No persistence, configuration file, or GUI — it's a console program by
  design, to keep the project focused and explainable.

## Author

### Dilkhush Raj
- **GitHub**: [@Dilkhush144](https://github.com/Dilkhush144)
- **LinkedIn**: [Dilkhush Raj](https://www.linkedin.com/in/dilkhushraj)
- **Email**: [kdilkhush844@gmail.com](mailto:kdilkhush844@gmail.com)

---

## License

MIT — see [LICENSE](LICENSE).
