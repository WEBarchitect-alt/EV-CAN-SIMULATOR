# can-bus-simulator
CAN Bus Telemetry Simulator

A C-based simulator that models CAN (Controller Area Network) bus telemetry for a vehicle, decoding raw CAN frames into human-readable engine, motor, and transmission data in real time.

Overview

This project simulates a vehicle's CAN bus by generating raw wire-level frames (CAN ID, DLC, payload bytes) at each clock tick, then decoding them into readable telemetry such as:

Engine: RPM, Throttle %, Coolant Temperature, Status
Motor: RPM
TCU (Transmission Control Unit): Gear, Vehicle Speed

The simulator also raises alerts (e.g. speed thresholds triggering gear shifts) as the simulation progresses.

Project Structure
can_project/
├── main_simulation.c   # Entry point; runs the simulation loop and clock ticks
├── ecu_common.h        # Shared structs, constants, and CAN ID definitions
└── README.md
Requirements
GCC (MinGW on Windows, or any standard GCC toolchain)
Building

From the project directory:

powershell
gcc main_simulation.c -o can_simulator

This produces can_simulator.exe (Windows) or can_simulator (Linux/macOS).

Running
powershell
.\can_simulator.exe

Sample output:

========================================================
        VEHICLE CAN BUS TELEMETRY SIMULATOR RUNNING
========================================================

--- [CLOCK TICK 1] ---------------------------------

>>> [RAW WIRE DUMP] Telemetry Packed on Bus:
CAN ID | DLC | Payload Hex Bytes
-------+-----+-----------------------------
0x101  | 3   | 04 9C 0F .. .. .. .. ..
0x102  | 2   | 73 00 .. .. .. .. .. ..
0x201  | 2   | 09 A3 .. .. .. .. .. ..
0x301  | 3   | 01 0B 90 .. .. .. .. ..

>>> [DECODED PANEL] Human-Readable Readout:
   [Engine] RPM: 1180, Throttle: 15%
   [Engine] Coolant Temp: 75.0 C, Status: OK
   [Motor]  RPM: 2467
   [TCU]    Gear: 1, Vehicle Speed: 29.60 km/h
Notes
Compiled binaries (*.exe, *.obj, *.o) are excluded from version control via .gitignore.
This is a simulation only — no real CAN hardware or bus interface is required.
