# Smart Pill Reminder and Medicine Tracker
## Final Exam Project Report
### Board: Arduino Uno

---

## 1. Introduction

### 1.1 Problem Statement

Medication non-adherence is a critical global health problem. According to the World Health Organization (WHO), approximately 50% of patients with chronic diseases do not take their medication as prescribed. This leads to disease complications, hospitalizations, and preventable deaths. Elderly patients and those with memory impairments are especially vulnerable.

### 1.2 Project Objective

To design and implement an Arduino Uno-based Smart Pill Reminder system that:
- Reminds patients to take medication at scheduled times
- Detects whether the pill box was opened (pill taken)
- Alerts caregivers via Serial Monitor if a dose is missed
- Logs all medication events locally for monitoring

### 1.3 Target Users

- Elderly patients living alone
- Patients with chronic diseases (diabetes, hypertension, heart disease)
- Family caregivers monitoring a patient remotely
- Healthcare providers tracking patient compliance

---

## 2. System Architecture

### 2.1 Block Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    SMART PILL REMINDER SYSTEM               │
│                                                             │
│  ┌──────────┐    ┌──────────────┐    ┌──────────────────┐  │
│  │ DS3231   │───►│              │───►│ Buzzer + LEDs    │  │
│  │ RTC      │    │  Arduino Uno │    │ (Local Alerts)   │  │
│  └──────────┘    │              │    └──────────────────┘  │
│                  │  Main        │                           │
│  ┌──────────┐    │  Controller  │    ┌──────────────────┐  │
│  │ Pill Box │───►│              │───►│ Serial Monitor   │  │
│  │ Switch   │    │              │    │ (Event Logging)  │  │
│  └──────────┘    │              │    └──────────────────┘  │
│                  │              │                           │
│  ┌──────────┐    │              │                           │
│  │ Confirm  │───►│              │                           │
│  │ Button   │    │              │                           │
│  └──────────┘    │              │                           │
│                  │              │                           │
│  ┌──────────┐    │              │                           │
│  │ Snooze   │───►│              │                           │
│  │ Button   │    └──────────────┘                           │
│  └──────────┘                                               │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Software Architecture

The firmware follows an event-driven loop architecture:

```
setup()
  ├── Initialize GPIO pins
  ├── Initialize DS3231 RTC
  └── Startup beep

loop()
  ├── Check for new day → reset states
  ├── [If reminder active]
  │     ├── Continue buzzer/LED pattern
  │     ├── Poll pill switch → confirmPillTaken()
  │     ├── Poll confirm button → confirmPillTaken()
  │     ├── Poll snooze button → snoozeReminder()
  │     └── Check timeout → mark missed + Serial alert
  └── [If no reminder]
        └── checkSchedule() → triggerReminder()
```

---

## 3. Hardware Design

### 3.1 Components and Justification

| Component     | Specification | Justification                                    |
|---------------|---------------|--------------------------------------------------|
| Arduino Uno   | ATmega328P    | Widely available, simple to program, 5V logic    |
| DS3231 RTC    | I2C, ±2ppm    | High accuracy, battery backup, temperature comp  |
| Active Buzzer | 5V, 85dB      | Audible alert without external oscillator        |
| LEDs (3×)     | 5mm, 20mA     | Visual color-coded indicator per dose time       |
| Reed Switch   | NO type       | Non-invasive pill box detection                  |
| Push Buttons  | Momentary NO  | User input for confirm and snooze                |

### 3.2 Power Consumption Estimate

| State              | Current Draw | Duration  |
|--------------------|-------------|-----------|
| Idle (WiFi active) | ~80 mA      | Most of day |
| Buzzer active      | ~120 mA     | Up to 5 min |
| LED on             | ~20 mA each | During alert |
| Deep sleep (future)| ~20 µA      | Optional  |

Estimated daily power: ~0.5 Wh (USB power bank lasts weeks)

---

## 4. Software Design

### 4.1 Key Data Structures

```cpp
// Medicine schedule (3 doses per day)
const int PILL_HOUR[3]   = { 8, 13, 20 };
const int PILL_MINUTE[3] = { 0,  0,  0 };

// State tracking
bool pillTaken[3]  = { false, false, false };
bool pillMissed[3] = { false, false, false };
int  snoozeExtraMinutes[3] = { 0, 0, 0 };
```

### 4.2 State Machine

```
         ┌─────────────────────────────────────────┐
         │              IDLE STATE                  │
         │  Waiting for scheduled pill time         │
         └──────────────┬──────────────────────────┘
                        │ Time matches schedule
                        ▼
         ┌─────────────────────────────────────────┐
         │           REMINDER ACTIVE                │
         │  Buzzer beeping, LED blinking            │
         │  Waiting up to 5 minutes                 │
         └──┬──────────────┬──────────────┬────────┘
            │              │              │
     Pill box         Confirm btn     Snooze btn
     opened           pressed         pressed
            │              │              │
            ▼              ▼              ▼
    ┌──────────┐   ┌──────────┐   ┌──────────────┐
    │ CONFIRMED│   │ CONFIRMED│   │   SNOOZED    │
    │ Log=1    │   │ Log=1    │   │ +10 min added│
    │ Notify   │   │ Notify   │   │ Back to IDLE │
    └──────────┘   └──────────┘   └──────────────┘
            │              │
            └──────┬───────┘
                   ▼
              Back to IDLE
                   
    [If 5 min timeout with no action]
                   ▼
         ┌─────────────────┐
         │     MISSED      │
         │  Log=0, Alert   │
         │  caregiver      │
         └─────────────────┘
```

### 4.3 Serial Monitor Output

All events are logged to the Serial Monitor at 9600 baud:

| Event          | Serial Output                                    |
|----------------|--------------------------------------------------|
| Reminder fires | `[REMINDER] >>> Morning pill time! <<<`          |
| Pill taken     | `[CONFIRM] Morning pill TAKEN. Good job!`        |
| Pill missed    | `[ALERT]  MISSED: Morning pill!`                 |
| Snoozed        | `[SNOOZE]  Morning snoozed. (2 left)`            |
| New day        | `[SYSTEM] New day – resetting pill states.`      |

---

## 5. Testing and Results

### 5.1 Test Cases

| Test ID | Test Description                    | Expected Result              | Status |
|---------|-------------------------------------|------------------------------|--------|
| TC-01   | RTC not connected                   | Error message, LED blink     | ✅ Pass |
| TC-02   | RTC lost power                      | Compile-time fallback set    | ✅ Pass |
| TC-03   | Pill time reached, no action        | Buzzer + LED activate        | ✅ Pass |
| TC-04   | Pill box opened during reminder     | Confirmed, Serial log        | ✅ Pass |
| TC-05   | Confirm button pressed              | Confirmed, 3 beeps           | ✅ Pass |
| TC-06   | Snooze button pressed               | 10 min delay added           | ✅ Pass |
| TC-07   | Snooze pressed 3× (max reached)     | Marked as missed             | ✅ Pass |
| TC-08   | No action for 5 minutes             | Missed alert on Serial       | ✅ Pass |
| TC-09   | Midnight reset                      | All states cleared           | ✅ Pass |
| TC-10   | Wokwi simulation                    | Morning reminder in ~10 sec  | ✅ Pass |

### 5.2 Limitations

- No wireless connectivity (local Serial Monitor only)
- No mobile notifications – caregiver must monitor Serial output
- Single pill box (multi-compartment requires hardware modification)
- Time must be set manually if RTC battery is replaced

---

## 6. Future Enhancements

1. **LCD Display** – Show current time and next pill schedule locally
2. **WiFi Module (ESP8266)** – Add wireless notifications via Blynk or Telegram
3. **Multiple Compartments** – Separate switches per compartment (Mon–Sun)
4. **Voice Reminder** – DFPlayer Mini module for audio messages
5. **Battery Backup** – 9V battery with low-battery detection
6. **OLED Screen** – Display medication name and countdown timer
7. **SD Card Logging** – Save adherence history to SD card
8. **Deep Sleep Mode** – Reduce power consumption between reminders
9. **Multi-patient** – One gateway managing multiple pill boxes

---

## 7. Conclusion

The Smart Pill Reminder and Medicine Tracker successfully demonstrates the application of embedded systems in healthcare. The system integrates real-time clock management, local alert mechanisms, and user interaction into a compact, low-cost device built on the Arduino Uno platform.

The device reliably triggers reminders at scheduled times, detects pill box interaction, handles user responses (confirm/snooze with a 3-snooze limit), and logs missed dose alerts to the Serial Monitor. All events are printed for local monitoring and caregiver awareness.

This project addresses a real-world healthcare problem and demonstrates competency in embedded C++ programming, hardware-software co-design, and state machine implementation on a microcontroller platform.

---

## 8. References

1. Arduino Uno R3 Datasheet – arduino.cc
2. ATmega328P Datasheet – Microchip Technology
3. DS3231 Datasheet – Maxim Integrated
4. Adafruit RTClib Library – github.com/adafruit/RTClib
5. WHO Report on Medication Adherence – who.int
6. Wokwi Arduino Simulator – wokwi.com
