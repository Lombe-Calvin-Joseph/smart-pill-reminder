/*
 * ============================================================
 *  Smart Pill Reminder and Medicine Tracker
 *  Author  : [Your Name]
 *  Date    : 2026
 *  Board   : ESP8266 (NodeMCU v1.0)
 *  Purpose : Remind patients to take medication, detect pill
 *            box opening, alert caregivers on missed doses,
 *            and log data to ThingSpeak cloud.
 * ============================================================
 */

// ─────────────────────────────────────────────
//  BLYNK CREDENTIALS  ← Change these
//  MUST be defined BEFORE including BlynkSimpleEsp8266.h
//  Get these from blynk.cloud → your Template
// ─────────────────────────────────────────────
#define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Smart Pill Reminder"
#define BLYNK_AUTH_TOKEN    "YOUR_BLYNK_AUTH_TOKEN"
#define BLYNK_PRINT Serial

// ─────────────────────────────────────────────
//  LIBRARIES
// ─────────────────────────────────────────────
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <RTClib.h>
#include <BlynkSimpleEsp8266.h>
#include <ThingSpeak.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// ─────────────────────────────────────────────
//  WiFi CREDENTIALS  ← Change these
// ─────────────────────────────────────────────
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const int   WIFI_MAX_RETRIES = 10;

// ─────────────────────────────────────────────
//  THINGSPEAK CREDENTIALS  ← Change these
// ─────────────────────────────────────────────
const unsigned long THINGSPEAK_CHANNEL_ID  = 0;        // e.g. 1234567
const char*         THINGSPEAK_WRITE_KEY   = "YOUR_THINGSPEAK_WRITE_KEY";

// ─────────────────────────────────────────────
//  PIN DEFINITIONS  (NodeMCU labels → GPIO)
// ─────────────────────────────────────────────
#define BUZZER          5   // D1 – active buzzer
#define LED_RED         4   // D2 – morning reminder
#define LED_YELLOW      0   // D3 – afternoon reminder
#define LED_GREEN       2   // D4 – evening reminder
#define PILL_SWITCH    14   // D5 – reed/micro-switch on pill box lid
#define CONFIRM_BUTTON 12   // D6 – manual "I took my pill" button
#define SNOOZE_BUTTON  13   // D7 – snooze 10 minutes

// ─────────────────────────────────────────────
//  MEDICINE SCHEDULE
//  Index 0 = Morning | 1 = Afternoon | 2 = Evening
// ─────────────────────────────────────────────
const int PILL_COUNT = 3;

const int PILL_HOUR[PILL_COUNT]   = {  8, 13, 20 };
const int PILL_MINUTE[PILL_COUNT] = {  0,  0,  0 };

const char* PILL_NAME[PILL_COUNT] = {
    "Morning",
    "Afternoon",
    "Evening"
};

const int PILL_LED[PILL_COUNT] = { LED_RED, LED_YELLOW, LED_GREEN };

// ─────────────────────────────────────────────
//  TIMING CONSTANTS
// ─────────────────────────────────────────────
#define REMINDER_WINDOW_MIN   5       // minutes to wait for confirmation
#define SNOOZE_DURATION_MIN  10       // snooze delay in minutes
#define BUZZER_ON_MS        500       // buzzer beep ON duration
#define BUZZER_OFF_MS       500       // buzzer beep OFF duration
#define LOOP_DELAY_MS       500       // main loop delay

// ─────────────────────────────────────────────
//  GLOBAL OBJECTS
// ─────────────────────────────────────────────
RTC_DS3231  rtc;
WiFiClient  wifiClient;
WiFiUDP     ntpUDP;
NTPClient   timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // UTC offset 0; adjust as needed

// ─────────────────────────────────────────────
//  STATE VARIABLES
// ─────────────────────────────────────────────
bool  pillTaken[PILL_COUNT]       = { false, false, false };
bool  pillMissed[PILL_COUNT]      = { false, false, false };
bool  reminderActive              = false;
int   activePillIndex             = -1;
unsigned long reminderStartMillis = 0;
int   snoozeExtraMinutes[PILL_COUNT] = { 0, 0, 0 };

// Track which day the pill states were last reset
int   lastResetDay = -1;

// ─────────────────────────────────────────────
//  FORWARD DECLARATIONS
// ─────────────────────────────────────────────
void connectWiFi();
void initRTC();
void syncTimeFromNTP();
void checkSchedule();
void triggerReminder(int pillIndex);
void confirmPillTaken();
void snoozeReminder();
void sendToThingSpeak(int pillType, int status);
void resetDailyState();
String getFormattedTime();
void buzzerBeep(int times);
void setLED(int pin, bool state);
void allLEDsOff();
void allOutputsOff();

// ─────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n========================================");
    Serial.println("  Smart Pill Reminder - Booting...");
    Serial.println("========================================");

    // Output pins
    pinMode(BUZZER,         OUTPUT);
    pinMode(LED_RED,        OUTPUT);
    pinMode(LED_YELLOW,     OUTPUT);
    pinMode(LED_GREEN,      OUTPUT);

    // Input pins with internal pull-up (LOW = pressed/triggered)
    pinMode(PILL_SWITCH,    INPUT_PULLUP);
    pinMode(CONFIRM_BUTTON, INPUT_PULLUP);
    pinMode(SNOOZE_BUTTON,  INPUT_PULLUP);

    allOutputsOff();

    connectWiFi();
    initRTC();
    syncTimeFromNTP();

    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);
    ThingSpeak.begin(wifiClient);

    // Startup confirmation beep
    buzzerBeep(2);

    Serial.println("\n[SYSTEM] Startup complete.");
    Serial.print("[SYSTEM] Current time: ");
    Serial.println(getFormattedTime());
    Serial.println("========================================\n");
}

// ─────────────────────────────────────────────
//  MAIN LOOP
// ─────────────────────────────────────────────
void loop() {
    Blynk.run();
    timeClient.update();

    DateTime now = rtc.now();

    // Reset pill states at midnight each new day
    if (now.day() != lastResetDay) {
        resetDailyState();
        lastResetDay = now.day();
    }

    // Check if a reminder is currently active
    if (reminderActive) {
        unsigned long elapsed = millis() - reminderStartMillis;

        // Buzz and blink while waiting for confirmation
        digitalWrite(BUZZER, HIGH);
        delay(BUZZER_ON_MS);
        digitalWrite(BUZZER, LOW);
        delay(BUZZER_OFF_MS);
        digitalWrite(PILL_LED[activePillIndex], !digitalRead(PILL_LED[activePillIndex]));

        // Check pill box switch (physical confirmation)
        if (digitalRead(PILL_SWITCH) == LOW) {
            Serial.println("[EVENT] Pill box opened – auto-confirming.");
            confirmPillTaken();
            return;
        }

        // Check manual confirm button
        if (digitalRead(CONFIRM_BUTTON) == LOW) {
            delay(50); // debounce
            if (digitalRead(CONFIRM_BUTTON) == LOW) {
                Serial.println("[EVENT] Confirm button pressed.");
                confirmPillTaken();
                return;
            }
        }

        // Check snooze button
        if (digitalRead(SNOOZE_BUTTON) == LOW) {
            delay(50); // debounce
            if (digitalRead(SNOOZE_BUTTON) == LOW) {
                Serial.println("[EVENT] Snooze button pressed.");
                snoozeReminder();
                return;
            }
        }

        // Timeout – pill was missed
        if (elapsed >= (unsigned long)(REMINDER_WINDOW_MIN * 60 * 1000UL)) {
            Serial.println("[ALERT] Reminder timeout – pill MISSED.");
            pillMissed[activePillIndex] = true;
            allOutputsOff();
            reminderActive = false;

            String msg = String(PILL_NAME[activePillIndex]) + " pill was MISSED! Please check on patient.";
            Blynk.logEvent("pill_missed", msg);
            Serial.println("[BLYNK] Missed alert sent: " + msg);

            sendToThingSpeak(activePillIndex, 0); // 0 = missed
            activePillIndex = -1;
        }

    } else {
        // No active reminder – check schedule
        checkSchedule();
    }

    delay(LOOP_DELAY_MS);
}

// ─────────────────────────────────────────────
//  WiFi CONNECTION
// ─────────────────────────────────────────────
void connectWiFi() {
    Serial.print("[WiFi] Connecting to: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_MAX_RETRIES) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WiFi] Connected!");
        Serial.print("[WiFi] IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n[WiFi] FAILED to connect. Running in offline mode.");
        // Device continues to work offline (local reminders still function)
    }
}

// ─────────────────────────────────────────────
//  RTC INITIALIZATION
// ─────────────────────────────────────────────
void initRTC() {
    Wire.begin();
    if (!rtc.begin()) {
        Serial.println("[RTC] ERROR: DS3231 not detected! Check wiring.");
        // Blink all LEDs to signal hardware error
        for (int i = 0; i < 10; i++) {
            digitalWrite(LED_RED,    HIGH);
            digitalWrite(LED_YELLOW, HIGH);
            digitalWrite(LED_GREEN,  HIGH);
            delay(300);
            allLEDsOff();
            delay(300);
        }
        // Continue anyway – time checks will fail gracefully
        return;
    }

    if (rtc.lostPower()) {
        Serial.println("[RTC] WARNING: RTC lost power, time may be wrong.");
        Serial.println("[RTC] Setting compile time as fallback...");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    Serial.println("[RTC] DS3231 initialized OK.");
    Serial.print("[RTC] Current time: ");
    Serial.println(getFormattedTime());
}

// ─────────────────────────────────────────────
//  NTP TIME SYNC
// ─────────────────────────────────────────────
void syncTimeFromNTP() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NTP] Skipping sync – no WiFi.");
        return;
    }

    Serial.println("[NTP] Syncing time from pool.ntp.org ...");
    timeClient.begin();
    timeClient.update();

    // NTP gives us epoch time; set RTC from it
    unsigned long epochTime = timeClient.getEpochTime();
    if (epochTime > 1000000000UL) {
        rtc.adjust(DateTime(epochTime));
        Serial.println("[NTP] RTC updated from NTP.");
        Serial.print("[NTP] New time: ");
        Serial.println(getFormattedTime());
    } else {
        Serial.println("[NTP] Sync failed – keeping existing RTC time.");
    }
}

// ─────────────────────────────────────────────
//  CHECK SCHEDULE
// ─────────────────────────────────────────────
void checkSchedule() {
    DateTime now = rtc.now();
    int currentHour   = now.hour();
    int currentMinute = now.minute();

    for (int i = 0; i < PILL_COUNT; i++) {
        if (pillTaken[i] || pillMissed[i]) continue; // already handled today

        int scheduledHour   = PILL_HOUR[i];
        int scheduledMinute = PILL_MINUTE[i] + snoozeExtraMinutes[i];

        // Normalize minutes overflow
        scheduledHour   += scheduledMinute / 60;
        scheduledMinute  = scheduledMinute % 60;

        if (currentHour == scheduledHour && currentMinute == scheduledMinute) {
            Serial.print("[SCHEDULE] Time match for: ");
            Serial.println(PILL_NAME[i]);
            triggerReminder(i);
            return; // handle one at a time
        }
    }
}

// ─────────────────────────────────────────────
//  TRIGGER REMINDER
// ─────────────────────────────────────────────
void triggerReminder(int pillIndex) {
    activePillIndex    = pillIndex;
    reminderActive     = true;
    reminderStartMillis = millis();

    Serial.print("[REMINDER] Triggering reminder for: ");
    Serial.println(PILL_NAME[pillIndex]);

    // Turn on corresponding LED
    setLED(PILL_LED[pillIndex], true);

    // Send Blynk push notification
    String msg = "Time to take your " + String(PILL_NAME[pillIndex]) + " medicine!";
    Blynk.logEvent("pill_reminder", msg);
    Serial.println("[BLYNK] Notification sent: " + msg);
}

// ─────────────────────────────────────────────
//  CONFIRM PILL TAKEN
// ─────────────────────────────────────────────
void confirmPillTaken() {
    if (activePillIndex < 0) return;

    pillTaken[activePillIndex] = true;
    reminderActive = false;
    allOutputsOff();

    // Confirmation beep pattern
    buzzerBeep(3);

    String msg = String(PILL_NAME[activePillIndex]) + " medicine taken. Good job!";
    Blynk.logEvent("pill_taken", msg);
    Serial.println("[CONFIRM] " + msg);

    sendToThingSpeak(activePillIndex, 1); // 1 = taken

    activePillIndex = -1;
}

// ─────────────────────────────────────────────
//  SNOOZE REMINDER
// ─────────────────────────────────────────────
void snoozeReminder() {
    if (activePillIndex < 0) return;

    snoozeExtraMinutes[activePillIndex] += SNOOZE_DURATION_MIN;
    reminderActive = false;
    allOutputsOff();

    String msg = String(PILL_NAME[activePillIndex]) + " reminder snoozed for " +
                 String(SNOOZE_DURATION_MIN) + " minutes.";
    Blynk.logEvent("pill_snoozed", msg);
    Serial.println("[SNOOZE] " + msg);

    activePillIndex = -1;
}

// ─────────────────────────────────────────────
//  SEND TO THINGSPEAK
//  Field 1 = Morning, Field 2 = Afternoon, Field 3 = Evening
//  Value  1 = Taken,  Value  0 = Missed
// ─────────────────────────────────────────────
void sendToThingSpeak(int pillType, int status) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[ThingSpeak] No WiFi – skipping log.");
        return;
    }

    int field = pillType + 1; // fields are 1-indexed
    ThingSpeak.setField(field, status);

    int result = ThingSpeak.writeFields(THINGSPEAK_CHANNEL_ID, THINGSPEAK_WRITE_KEY);

    if (result == 200) {
        Serial.print("[ThingSpeak] Logged field ");
        Serial.print(field);
        Serial.print(" = ");
        Serial.println(status);
    } else {
        Serial.print("[ThingSpeak] Error code: ");
        Serial.println(result);
    }
}

// ─────────────────────────────────────────────
//  RESET DAILY STATE (called at midnight)
// ─────────────────────────────────────────────
void resetDailyState() {
    Serial.println("[SYSTEM] New day – resetting pill states.");
    for (int i = 0; i < PILL_COUNT; i++) {
        pillTaken[i]          = false;
        pillMissed[i]         = false;
        snoozeExtraMinutes[i] = 0;
    }
    reminderActive  = false;
    activePillIndex = -1;
    allOutputsOff();
}

// ─────────────────────────────────────────────
//  GET FORMATTED TIME STRING
// ─────────────────────────────────────────────
String getFormattedTime() {
    DateTime now = rtc.now();

    char buf[25];
    snprintf(buf, sizeof(buf),
             "%02d:%02d:%02d - %02d/%02d/%04d",
             now.hour(), now.minute(), now.second(),
             now.day(),  now.month(),  now.year());
    return String(buf);
}

// ─────────────────────────────────────────────
//  HELPER – BUZZER BEEP PATTERN
// ─────────────────────────────────────────────
void buzzerBeep(int times) {
    for (int i = 0; i < times; i++) {
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(BUZZER, LOW);
        delay(150);
    }
}

// ─────────────────────────────────────────────
//  HELPER – LED CONTROL
// ─────────────────────────────────────────────
void setLED(int pin, bool state) {
    digitalWrite(pin, state ? HIGH : LOW);
}

void allLEDsOff() {
    digitalWrite(LED_RED,    LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN,  LOW);
}

void allOutputsOff() {
    allLEDsOff();
    digitalWrite(BUZZER, LOW);
}
