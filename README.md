# Smart Pill Reminder and Medicine Tracker

An IoT device built on ESP8266 (NodeMCU) that reminds patients to take medication,
detects pill box opening, and alerts caregivers via mobile if a dose is missed.

## Quick Start

1. Open this folder in VS Code with PlatformIO installed
2. Edit `src/main.cpp` – fill in WiFi, Blynk, and ThingSpeak credentials
3. Wire components per `docs/circuit_diagram.md`
4. Click Build → Upload
5. Open Serial Monitor at 115200 baud

## File Structure

```
SmartPillReminder/
├── src/
│   └── main.cpp              ← All firmware code
├── docs/
│   ├── circuit_diagram.md    ← Wiring tables and ASCII diagrams
│   ├── setup_instructions.md ← Step-by-step setup guide
│   └── project_report.md     ← Full academic project report
├── platformio.ini            ← Build config and library dependencies
└── README.md                 ← This file
```

## Hardware

| Pin  | GPIO | Component         |
|------|------|-------------------|
| D1   | 5    | Buzzer            |
| D2   | 4    | LED Red (morning) |
| D3   | 0    | LED Yellow (noon) |
| D4   | 2    | LED Green (eve)   |
| D5   | 14   | Pill box switch   |
| D6   | 12   | Confirm button    |
| D7   | 13   | Snooze button     |
| D1/D2| 5/4  | DS3231 SCL/SDA    |

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
- Snooze button → delays reminder by 10 minutes
- No response within 5 minutes → logged as **missed**, caregiver alerted via Blynk
- All events logged to ThingSpeak (Field 1/2/3 = Morning/Afternoon/Evening)
- States reset automatically at midnight each day

## Dependencies

All auto-installed by PlatformIO via `platformio.ini`:
- RTClib (Adafruit)
- Blynk
- ThingSpeak
- NTPClient
