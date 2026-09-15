# Diagram: Emergency Detection -> Priority Mode -> Normal Mode

```
                     ┌───────────────────────────┐
                     │ EmergencyVehicleDetector    │
                     │ reportEmergency(direction)  │
                     └─────────────┬──────────────┘
                                   │ sets Lane::emergencyActive_ = true
                                   ▼
        (checked every tick, even mid-phase, by TrafficController)
                                   │
                                   ▼
                     ┌───────────────────────────┐
                     │ Is a normal green phase     │
                     │ currently running?          │
                     └─────┬─────────────────┬────┘
                       yes │                 │ no
                           ▼                 ▼
              ┌─────────────────────┐   ┌─────────────────────┐
              │ Interrupt it:        │   │ Skip straight to     │
              │ GREEN -> YELLOW ->    │   │ emergency phase       │
              │ RED for that lane    │   │                       │
              └──────────┬───────────┘   └──────────┬───────────┘
                         └───────────────┬───────────┘
                                         ▼
                     ┌───────────────────────────┐
                     │ setAllRedExcept(emergency   │
                     │ direction) — every other     │
                     │ lane forced to RED first      │
                     └─────────────┬─────────────┘
                                   ▼
                     ┌───────────────────────────┐
                     │ Emergency lane -> GREEN      │
                     │ held for fixed clearance      │
                     │ time (not density-based)      │
                     └─────────────┬─────────────┘
                                   ▼
                     ┌───────────────────────────┐
                     │ GREEN -> YELLOW -> RED        │
                     │ clearEmergency(direction)      │
                     └─────────────┬─────────────┘
                                   ▼
                     ┌───────────────────────────┐
                     │ Any more emergencies         │
                     │ queued? (multiple requests)   │
                     └─────┬─────────────────┬────┘
                       yes │                 │ no
                           │                 ▼
                           │      ┌───────────────────────────┐
                           │      │ Resume normal adaptive       │
                           │      │ round-robin control from      │
                           │      │ where it left off             │
                           │      └───────────────────────────┘
                           └─────────────► (repeat emergency phase)
```

This maps directly onto
[`TrafficController::drainEmergencyQueue`](../../src/TrafficController.cpp)
and `TrafficController::serveDirectionEmergency`. `drainEmergencyQueue()` is
a loop specifically so that if two lanes report an emergency at
(near-)the same time, both are served, one after another, before the
system goes back to normal density-based scheduling.
