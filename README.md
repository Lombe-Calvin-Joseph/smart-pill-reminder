# Smart Pill Reminder and Medicine Tracker

An Arduino Uno device that reminds patients to take medication at scheduled times,
detects pill box opening, and logs all events to the Serial Monitor.

## Quick Start

1. Open this folder in VS Code with PlatformIO installed
2. Wire components per `docs/circuit_diagram.md`
3. Click Build → Upload
4. Open Serial Monitor at **9600 baud**

## File Structure

```
SmartPillReminder/
├── src/
│   └── main.cpp              ← Firmware (Arduino Uno)
├── wokwi/
│   ├── sketch.ino            ← Wokwi simulation sketch
│   └── diagram.json          ← Wokwi circuit diagram
├── diagram.json              ← Root Wokwi diagram (same as wokwi/)
├── docs/
│   ├── circuit_diagram.md    ← Wiring tables and ASCII diagrams
│   ├── setup_instructions.md ← Step-by-step setup guide
│   └── project_report.md     ← Full academic project report
├── platformio.ini            ← Build config and library dependencies
└── README.md                 ← This file
```

## Hardware

| Pin | Component              |
|-----|------------------------|
| D2  | LED Green (evening)    |
| D3  | LED Yellow (afternoon) |
| D4  | LED Red (morning)      |
| D5  | Active Buzzer          |
| D6  | Snooze Button          |
| D7  | Confirm Button         |
| D8  | Pill Box Switch        |
| A4  | DS3231 SDA (I2C)       |
| A5  | DS3231 SCL (I2C)       |

## Default Schedule

| Dose      | Time  |
|-----------|-------|
| Morning   | 08:00 |
| Afternoon | 13:00 |
| Evening   | 20:00 |

Change in `main.cpp` → `PILL_HOUR[]` and `PILL_MINUTE[]` arrays.

## Behavior

- Reminder triggers buzzer + LED at scheduled time
- Patient opens pill box OR presses confirm → logged as **taken**
- Snooze button → delays reminder by 10 minutes (max 3 snoozes)
- No response within 5 minutes → logged as **missed**
- All events printed to Serial Monitor at 9600 baud
- States reset automatically at midnight each day

## Wokwi Simulation

Open `wokwi/diagram.json` at [wokwi.com](https://wokwi.com) to simulate.
The RTC is pre-set to 07:59:50 so the morning reminder fires ~10 seconds after start.

## Dependencies

Auto-installed by PlatformIO via `platformio.ini`:
- RTClib (Adafruit)
