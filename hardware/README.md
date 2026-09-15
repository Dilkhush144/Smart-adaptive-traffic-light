# Hardware Notes (Not Implemented — Design Notes Only)

**Architecture Note:** the main version of this project is a
**console simulation**. No physical sensors, LEDs, or microcontroller code
are included or have been tested against real hardware. This file only
describes *how* the existing C++ design could be connected to real
hardware later — it is documentation of a possible next step, not a
claim that it has been built.

## Why the current design ports reasonably well

The core logic (`TrafficLight`, `Lane`, `TrafficController`,
`EmergencyVehicleDetector`) never talks to the console directly and never
assumes `Sensor` is simulated — it only calls
`Sensor::readVehicleCount()`. That means, in principle:

- `Sensor` could be reimplemented to read an IR/ultrasonic distance
  sensor or an inductive loop detector instead of generating a random
  number, **without changing `Lane`, `TrafficController`, or the
  scheduling algorithm at all**.
- `TrafficLight::setState()` could be extended to also toggle real GPIO
  pins (e.g. via `digitalWrite` on Arduino/ESP32) inside the same
  function that currently just stores the enum value.
- `EmergencyVehicleDetector::reportEmergency()` could be called from an
  interrupt handler wired to an RF receiver or a siren-detecting
  microphone module instead of the demo injector thread in `main.cpp`.

## What would actually be needed for a physical build

- Microcontroller: Arduino Uno/Mega or ESP32.
- 4x sets of RED/YELLOW/GREEN LEDs (or real signal heads) with current-limiting resistors.
- 4x IR break-beam or ultrasonic sensors for vehicle counting, or inductive loop detectors.
- An emergency-vehicle signalling method — e.g. a simple RF transmitter/receiver pair, since a production system would use something like RF/RFID or siren detection rather than a software timer.
- If porting to Arduino/ESP32: `std::thread` and `std::mutex` are not
  available in the standard Arduino core, so the multithreaded design
  here would become a single-loop, interrupt-driven design instead
  (the algorithm — density → duration, emergency → priority — stays the
  same; only the concurrency mechanism changes).

## Status

No Arduino/ESP32 sketch is included in this repository. This document
exists so the design is clearly extensible, and so it's honest about
where the "embedded" story currently stops: the algorithm and class
design are ready for it, the physical build is not implemented.
