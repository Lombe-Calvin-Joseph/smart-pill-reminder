# Smart Pill Reminder and Medicine Tracker
## Final Exam Project Report

---

## 1. Introduction

### 1.1 Problem Statement

Medication non-adherence is a critical global health problem. According to the World Health Organization (WHO), approximately 50% of patients with chronic diseases do not take their medication as prescribed. This leads to disease complications, hospitalizations, and preventable deaths. Elderly patients and those with memory impairments are especially vulnerable.

### 1.2 Project Objective

To design and implement an IoT-based Smart Pill Reminder system that:
- Reminds patients to take medication at scheduled times
- Detects whether the pill box was opened (pill taken)
- Alerts caregivers via mobile notification if a dose is missed
- Logs medication adherence data to the cloud for healthcare monitoring

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
│  │ RTC      │    │  ESP8266     │    │ (Local Alerts)   │  │
│  └──────────┘    │  NodeMCU     │    └──────────────────┘  │
│                  │              │                           │
│  ┌──────────┐    │  Main        │    ┌──────────────────┐  │
│  │ Pill Box │───►│  Controller  │───►│ Blynk Cloud      │  │
│  │ Switch   │    │              │    │ (Push Notify)    │  │
│  └──────────┘    │              │    └──────────────────┘  │
│                  │              │                           │
│  ┌──────────┐    │              │    ┌──────────────────┐  │
│  │ Confirm  │───►│              │───►│ ThingSpeak Cloud │  │
│  │ Button   │    │              │    │ (Data Logging)   │  │
│  └──────────┘    │              │    └──────────────────┘  │
│                  │              │                           │
│  ┌──────────┐    │              │    ┌──────────────────┐  │
│  │ Snooze   │───►│              │    │ NTP Server       │  │
│  │ Button   │    └──────────────┘◄───│ (Time Sync)      │  │
│  └──────────┘                        └──────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Software Architecture

The firmware follows an event-driven loop architecture:

```
setup()
  ├── Initialize GPIO pins
  ├── Connect to WiFi (with retry)
  ├── Initialize DS3231 RTC
  ├── Sync time from NTP
  ├── Connect to Blynk
  └── Initialize ThingSpeak

loop()
  ├── Run Blynk (keep-alive)
  ├── Update NTP client
  ├── Check for new day → reset states
  ├── [If reminder active]
  │     ├── Continue buzzer/LED pattern
  │     ├── Poll pill switch → confirmPillTaken()
  │     ├── Poll confirm button → confirmPillTaken()
  │     ├── Poll snooze button → snoozeReminder()
  │     └── Check timeout → mark missed + alert
  └── [If no reminder]
        └── checkSchedule() → triggerReminder()
```

---

## 3. Hardware Design

### 3.1 Components and Justification

| Component     | Specification | Justification                                    |
|---------------|---------------|--------------------------------------------------|
| ESP8266       | NodeMCU v1.0  | Built-in WiFi, sufficient GPIO, low cost         |
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

### 4.3 Cloud Integration

**Blynk IoT:**
- Used for real-time push notifications to caregiver's smartphone
- Events: `pill_reminder`, `pill_taken`, `pill_missed`, `pill_snoozed`
- Free tier supports up to 5 devices

**ThingSpeak:**
- Used for long-term data logging and analytics
- Each field stores 0 (missed) or 1 (taken) per dose
- Data can be visualized as charts on ThingSpeak dashboard
- Free tier: 3 million messages/year, 15-second update interval

---

## 5. Testing and Results

### 5.1 Test Cases

| Test ID | Test Description                    | Expected Result              | Status |
|---------|-------------------------------------|------------------------------|--------|
| TC-01   | WiFi connection with correct creds  | Connected, IP printed        | ✅ Pass |
| TC-02   | WiFi with wrong password            | Retry 10×, offline mode      | ✅ Pass |
| TC-03   | RTC not connected                   | Error message, LED blink     | ✅ Pass |
| TC-04   | Pill time reached, no action        | Buzzer + LED activate        | ✅ Pass |
| TC-05   | Pill box opened during reminder     | Confirmed, ThingSpeak log=1  | ✅ Pass |
| TC-06   | Confirm button pressed              | Confirmed, Blynk notified    | ✅ Pass |
| TC-07   | Snooze button pressed               | 10 min delay, Blynk notified | ✅ Pass |
| TC-08   | No action for 5 minutes             | Missed alert sent, log=0     | ✅ Pass |
| TC-09   | Midnight reset                      | All states cleared           | ✅ Pass |
| TC-10   | NTP sync on boot                    | RTC updated from internet    | ✅ Pass |

### 5.2 Limitations

- Requires 2.4GHz WiFi (ESP8266 does not support 5GHz)
- ThingSpeak free tier has 15-second minimum update interval
- No local display (LCD can be added as future enhancement)
- Single pill box (multi-compartment box requires hardware modification)

---

## 6. Future Enhancements

1. **LCD Display** – Show current time and next pill schedule locally
2. **OLED Screen** – Display medication name and countdown timer
3. **Multiple Compartments** – Separate switches per compartment (Mon–Sun)
4. **Voice Reminder** – DFPlayer Mini module for audio messages
5. **Battery Backup** – LiPo battery with charging circuit
6. **Web Dashboard** – Custom React dashboard instead of ThingSpeak
7. **Telegram Bot** – Alternative to Blynk for caregiver alerts
8. **Deep Sleep Mode** – Reduce power consumption between reminders
9. **OTA Updates** – Over-the-air firmware updates via WiFi
10. **Multi-patient** – One gateway managing multiple pill boxes

---

## 7. Conclusion

The Smart Pill Reminder and Medicine Tracker successfully demonstrates the application of IoT technology in healthcare. The system integrates real-time clock management, WiFi connectivity, cloud notifications, and data logging into a compact, low-cost device built on the ESP8266 platform.

The device reliably triggers reminders at scheduled times, detects pill box interaction, handles user responses (confirm/snooze), and escalates to caregiver alerts when doses are missed. All events are logged to ThingSpeak for long-term adherence tracking.

This project addresses a real-world healthcare problem and demonstrates competency in embedded C++ programming, IoT protocols, cloud platform integration, and hardware-software co-design.

---

## 8. References

1. ESP8266 Technical Reference – Espressif Systems
2. DS3231 Datasheet – Maxim Integrated
3. Blynk IoT Documentation – blynk.cloud/documentation
4. ThingSpeak API Documentation – mathworks.com/help/thingspeak
5. WHO Report on Medication Adherence – who.int
6. Arduino RTClib Library – github.com/adafruit/RTClib
