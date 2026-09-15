# Diagram: Sensor Input -> Adaptive Signal Timing

Plain-text flowchart (renders fine on GitHub, no image tooling required).

```
 ┌───────────┐   ┌───────────┐   ┌───────────┐   ┌───────────┐
 │  Sensor   │   │  Sensor   │   │  Sensor   │   │  Sensor   │
 │  (NORTH)  │   │  (SOUTH)  │   │  (EAST)   │   │  (WEST)   │
 └─────┬─────┘   └─────┬─────┘   └─────┬─────┘   └─────┬─────┘
       │ readVehicleCount()             (each on its own
       ▼                                 background thread)
 ┌───────────┐   ┌───────────┐   ┌───────────┐   ┌───────────┐
 │   Lane    │   │   Lane    │   │   Lane    │   │   Lane    │
 │  (count,  │   │  (count,  │   │  (count,  │   │  (count,  │
 │  density) │   │  density) │   │  density) │   │  density) │
 └─────┬─────┘   └─────┬─────┘   └─────┬─────┘   └─────┬─────┘
       └────────────────┴────────┬───────┴────────────────┘
                                  ▼
                     ┌─────────────────────────┐
                     │    TrafficController      │
                     │  1. pick next direction   │
                     │     (round-robin order)   │
                     │  2. read that lane's       │
                     │     density                │
                     │  3. green = min + density  │
                     │     * (max - min)           │
                     └─────────────┬─────────────┘
                                  ▼
                     ┌─────────────────────────┐
                     │   TrafficLight (GREEN)    │
                     │   held for computed        │
                     │   duration, then           │
                     │   YELLOW -> RED             │
                     └─────────────────────────┘
```

**Read this as:** each lane's Sensor keeps feeding fresh vehicle counts into
its Lane on a background thread. When it's that lane's turn in the
round-robin rotation, TrafficController reads the *current* density and
plugs it into the linear formula in
[`TrafficController::calculateGreenDuration`](../../src/TrafficController.cpp)
to decide how long the green light stays on.
