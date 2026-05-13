# Setup Instructions – Smart Pill Reminder

## Prerequisites

- Windows / macOS / Linux PC
- USB Micro-B cable
- Internet connection (for library downloads)

---

## Step 1 – Install Development Environment

### Option A: PlatformIO (Recommended)

1. Install [VS Code](https://code.visualstudio.com/)
2. Open VS Code → Extensions → search **PlatformIO IDE** → Install
3. Restart VS Code
4. Open the `SmartPillReminder/` folder in VS Code
5. PlatformIO will auto-detect `platformio.ini` and download all libraries

### Option B: Arduino IDE

1. Download [Arduino IDE 2.x](https://www.arduino.cc/en/software)
2. Open **File → Preferences** → add this URL to "Additional Boards Manager URLs":
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Go to **Tools → Board → Boards Manager** → search `esp8266` → Install
4. Install these libraries via **Sketch → Include Library → Manage Libraries**:
   - `RTClib` by Adafruit
   - `Blynk` by Volodymyr Shymanskyy
   - `ThingSpeak` by MathWorks
   - `NTPClient` by Fabrice Weinberg

---

## Step 2 – Configure Credentials

Open `src/main.cpp` and update these lines at the top:

```cpp
const char* WIFI_SSID     = "YOUR_WIFI_SSID";       // ← your WiFi name
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";   // ← your WiFi password

char BLYNK_AUTH_TOKEN[] = "YOUR_BLYNK_AUTH_TOKEN";  // ← from Blynk app

const unsigned long THINGSPEAK_CHANNEL_ID = 0;      // ← your channel number
const char* THINGSPEAK_WRITE_KEY = "YOUR_KEY";      // ← your write API key
```

---

## Step 3 – Set Up Blynk

1. Download **Blynk IoT** app (iOS / Android)
2. Create a free account at [blynk.cloud](https://blynk.cloud)
3. Create a new **Template**:
   - Hardware: ESP8266
   - Connection: WiFi
4. Create these **Events** in the template:
   | Event Code    | Event Name         | Type  |
   |---------------|--------------------|-------|
   | pill_reminder | Pill Reminder      | Info  |
   | pill_taken    | Pill Taken         | Info  |
   | pill_missed   | Pill Missed        | Critical |
   | pill_snoozed  | Pill Snoozed       | Warning  |
5. Create a new **Device** from the template
6. Copy the **Auth Token** → paste into `main.cpp`
7. Enable **Push Notifications** in the app settings

---

## Step 4 – Set Up ThingSpeak

1. Create a free account at [thingspeak.com](https://thingspeak.com)
2. Create a new **Channel** with these fields:
   | Field   | Label              |
   |---------|--------------------|
   | Field 1 | Morning Pill       |
   | Field 2 | Afternoon Pill     |
   | Field 3 | Evening Pill       |
3. Go to **API Keys** tab → copy the **Write API Key**
4. Copy the **Channel ID** (number in the URL)
5. Paste both into `main.cpp`

---

## Step 5 – Adjust Timezone (NTP)

In `main.cpp`, find this line:

```cpp
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);
```

Change the `0` to your UTC offset in seconds:

| Timezone        | Offset (seconds) |
|-----------------|------------------|
| UTC+0 (London)  | 0                |
| UTC+3 (Riyadh)  | 10800            |
| UTC+5:30 (India)| 19800            |
| UTC+8 (Malaysia)| 28800            |
| UTC-5 (New York)| -18000           |

Example for Malaysia (UTC+8):
```cpp
NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800, 60000);
```

---

## Step 6 – Adjust Medicine Schedule

In `main.cpp`, find and edit:

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

## Step 7 – Build and Upload

### PlatformIO
1. Click the **Build** button (checkmark icon) in the bottom toolbar
2. Connect NodeMCU via USB
3. Click **Upload** (right arrow icon)
4. Open **Serial Monitor** (plug icon) at 115200 baud

### Arduino IDE
1. Select **Tools → Board → NodeMCU 1.0 (ESP-12E Module)**
2. Select **Tools → Port → COMx** (your device port)
3. Click **Upload** (right arrow)
4. Open **Tools → Serial Monitor** at 115200 baud

---

## Step 8 – Verify Operation

Watch the Serial Monitor output. You should see:

```
========================================
  Smart Pill Reminder - Booting...
========================================
[WiFi] Connecting to: YOUR_WIFI_SSID
....
[WiFi] Connected!
[WiFi] IP Address: 192.168.1.xxx
[RTC] DS3231 initialized OK.
[RTC] Current time: 08:00:00 - 13/05/2026
[NTP] Syncing time from pool.ntp.org ...
[NTP] RTC updated from NTP.
[SYSTEM] Startup complete.
========================================
```

---

## Troubleshooting

| Problem                        | Solution                                              |
|--------------------------------|-------------------------------------------------------|
| WiFi not connecting            | Check SSID/password, ensure 2.4GHz network           |
| RTC not detected               | Check SDA/SCL wiring, ensure 3.3V power to DS3231    |
| Blynk not connecting           | Verify auth token, check internet connection          |
| ThingSpeak error code 0        | Check channel ID and write key                        |
| No buzzer sound                | Verify D1 wiring, check if buzzer is active type     |
| LEDs not lighting              | Check 220Ω resistors, verify LED polarity            |
| Buttons not responding         | Ensure buttons connect pin to GND (not VCC)          |
| Time is wrong                  | Check NTP timezone offset, verify RTC battery        |
| Upload fails                   | Hold FLASH button on NodeMCU during upload start     |
