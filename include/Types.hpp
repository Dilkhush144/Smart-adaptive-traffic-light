#pragma once
#include <string>

// ---------------------------------------------------------------------------
// Types.hpp
//
// Shared, project-wide types. Kept in one small header so every class can
// speak the same vocabulary (Direction, LightState) without depending on
// each other's headers just to get an enum.
// ---------------------------------------------------------------------------

/// One of the four approaches at the intersection.
enum class Direction {
    NORTH,
    SOUTH,
    EAST,
    WEST
};

/// The three physical states a traffic light can show.
enum class LightState {
    RED,
    YELLOW,
    GREEN
};

std::string toString(Direction d);
std::string toString(LightState s);
