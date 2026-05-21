# Setup Instructions – Smart Pill Reminder

## Prerequisites

- Windows / macOS / Linux PC
- USB Type-B cable (Arduino Uno)
- Internet connection (for library downloads)

---

## Step 1 – Install Development Environment

### Option A: PlatformIO (Recommended)

1. Install [VS Code](https://code.visualstudio.com/)
2. Open VS Code → Extensions → search **PlatformIO IDE** → Install
3. Restart VS Code
4. Open the `SmartPillReminder/` folder in VS Code
5. PlatformIO will auto-detect `platformio.ini` and download RTClib

### Option B: Arduino IDE

1. Download [Arduino IDE 2.x](https://www.arduino.cc/en/software)
2. Install the **RTClib** library via **Sketch → Include Library → Manage Libraries**
   - Search `RTClib` by Adafruit → Install

---

## Step 2 – Adjust Medicine Schedule (Optional)

Open `src/main.cpp` and edit the schedule arrays:

```cpp
const int PILL_HOUR[PILL_COUNT]   = {  8, 13, 20 };
const int PILL_MINUTE[PILL_COUNT] = {  0,  0,  0 };
```

Example – change afternoon pill to 1:30 PM:
```cpp
const int PILL_HOUR[PILL_COUNT]   = {  8, 13, 20 };
const int PILL_MINUTE[PILL_COUNT] = {  0, 30,  0 };
```

---

## Step 3 – Set the RTC Time

The RTC is automatically set to the **compile time** on first upload (or after battery loss).
To set a specific time, temporarily add this line inside `setup()` after `initRTC()`:

```cpp
rtc.adjust(DateTime(2026, 5, 21, 8, 0, 0));  // YYYY, M, D, H, Min, Sec
```

Remove it after the first upload so the time is not reset on every reboot.

---

## Step 4 – Build and Upload

### PlatformIO
1. Click the **Build** button (checkmark icon) in the bottom toolbar
2. Connect Arduino Uno via USB
3. Click **Upload** (right arrow icon)
4. Open **Serial Monitor** (plug icon) at **9600 baud**

### Arduino IDE
1. Select **Tools → Board → Arduino Uno**
2. Select **Tools → Port → COMx** (your device port)
3. Click **Upload** (right arrow)
4. Open **Tools → Serial Monitor** at **9600 baud**

---

## Step 5 – Verify Operation

Watch the Serial Monitor. You should see:

```
========================================
  Smart Pill Reminder - Booting...
  Board: Arduino Uno
========================================
[RTC] DS3231 initialized OK.
[SYSTEM] Startup complete.
[SYSTEM] Current time: 08:00:00 13/05/2026
========================================
```

When a reminder fires:
```
[REMINDER] >>> Morning pill time! <<<
[INFO] Waiting up to 5 min. Snoozes left: 3
```

When confirmed:
```
[CONFIRM] Morning pill TAKEN. Good job!
```

When missed:
```
[ALERT]  MISSED: Morning pill! Please check on patient.
```

---

## Step 6 – Run Wokwi Simulation

1. Go to [wokwi.com](https://wokwi.com) → **New Project → Arduino Uno**
2. Replace the default `sketch.ino` with the contents of `wokwi/sketch.ino`
3. Click the diagram editor and import `wokwi/diagram.json`
4. Click **Play** – the morning reminder fires in ~10 seconds
5. Use the on-screen buttons to test confirm, snooze, and pill box

---

## Troubleshooting

| Problem                  | Solution                                              |
|--------------------------|-------------------------------------------------------|
| RTC not detected         | Check A4/A5 wiring, ensure 5V power to DS3231        |
| No buzzer sound          | Verify D5 wiring, check if buzzer is active type     |
| LEDs not lighting        | Check 220Ω resistors, verify LED polarity            |
| Buttons not responding   | Ensure buttons connect pin to GND (not VCC)          |
| Time is wrong            | Set RTC manually (Step 3), check battery on DS3231   |
| Upload fails             | Check COM port, try pressing Reset on Uno            |
| Reminder never triggers  | Verify RTC time matches schedule in main.cpp         |
