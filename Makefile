CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude -pthread
SRC      := $(wildcard src/*.cpp)
OBJ      := $(patsubst src/%.cpp,build/%.o,$(SRC))
TARGET   := build/traffic_controller

TEST_SRC := tests/test_main.cpp \
            src/Types.cpp src/TrafficLight.cpp src/Sensor.cpp \
            src/Lane.cpp src/EmergencyVehicleDetector.cpp src/TrafficController.cpp
TEST_TARGET := build/run_tests

.PHONY: all run tests clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

run: $(TARGET)
	./$(TARGET)

tests: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC) | build
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_SRC)

clean:
	rm -rf build
