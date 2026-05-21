/*
 * ============================================================
 *  Smart Pill Reminder – WOKWI SIMULATION VERSION
 *  
 *  Differences from production code:
 *  - WiFi / Blynk / ThingSpeak replaced with Serial.println()
 *  - First pill triggers 10 seconds after boot (easy to test)
 *  - Uses DS1307 (Wokwi compatible) instead of DS3231
 * ============================================================
 */

#include <Wire.h>
#include <RTClib.h>

// ── Pin Definitions ──────────────────────────────────────────
#define BUZZER          5   // D1
#define LED_RED         4   // D2 – Morning
#define LED_YELLOW      0   // D3 – Afternoon
#define LED_GREEN       2   // D4 – Evening
#define PILL_SWITCH    14   // D5 – Pill box lid
#define CONFIRM_BUTTON 12   // D6 – Confirm taken
#define SNOOZE_BUTTON  13   // D7 – Snooze 10 min

// ── Schedule ─────────────────────────────────────────────────
const int PILL_COUNT = 3;
int PILL_HOUR[PILL_COUNT]   = { 8, 13, 20 };
int PILL_MINUTE[PILL_COUNT] = { 0,  0,  0 };
const char* PILL_NAME[PILL_COUNT] = { "Morning", "Afternoon", "Evening" };
const int   PILL_LED[PILL_COUNT]  = { LED_RED, LED_YELLOW, LED_GREEN };

// ── Timing ───────────────────────────────────────────────────
#define REMINDER_WINDOW_MS  (5UL * 60 * 1000)   // 5 minutes
#define SNOOZE_DURATION_MIN 10
#define LOOP_DELAY_MS       300
#define BUZZER_ON_MS        500
#define BUZZER_OFF_MS       500

// ── State ────────────────────────────────────────────────────
RTC_DS1307 rtc;

bool pillTaken[PILL_COUNT]  = { false, false, false };
bool pillMissed[PILL_COUNT] = { false, false, false };
int  snoozeExtra[PILL_COUNT]= { 0, 0, 0 };

bool reminderActive         = false;
int  activePill             = -1;
unsigned long reminderStart = 0;
int  lastResetDay           = -1;

// ── Helpers ──────────────────────────────────────────────────
void allOff() {
    digitalWrite(BUZZER,     LOW);
    digitalWrite(LED_RED,    LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN,  LOW);
}

void beep(int n) {
    for (int i = 0; i < n; i++) {
        digitalWrite(BUZZER, HIGH); delay(200);
        digitalWrite(BUZZER, LOW);  delay(150);
    }
}

String timeStr() {
    DateTime now = rtc.now();
    char buf[20];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             now.hour(), now.minute(), now.second());
    return String(buf);
}

// ── Notification stubs (replace with Blynk in production) ────
void notify(String msg) {
    Serial.println("[NOTIFY] " + msg);
}

void logCloud(int field, int value) {
    Serial.print("[CLOUD]  Field ");
    Serial.print(field + 1);
    Serial.print(" = ");
    Serial.println(value == 1 ? "TAKEN" : "MISSED");
}

// ── Core Logic ───────────────────────────────────────────────
void triggerReminder(int idx) {
    activePill    = idx;
    reminderActive = true;
    reminderStart  = millis();
    digitalWrite(PILL_LED[idx], HIGH);
    Serial.println("\n[REMINDER] >>> " + String(PILL_NAME[idx]) + " pill time! <<<");
    notify("Time to take your " + String(PILL_NAME[idx]) + " medicine!");
}

void confirmPillTaken() {
    pillTaken[activePill] = true;
    reminderActive = false;
    allOff();
    beep(3);
    Serial.println("[CONFIRM] " + String(PILL_NAME[activePill]) + " pill TAKEN.");
    notify(String(PILL_NAME[activePill]) + " medicine taken. Good job!");
    logCloud(activePill, 1);
    activePill = -1;
}

void snoozeReminder() {
    snoozeExtra[activePill] += SNOOZE_DURATION_MIN;
    reminderActive = false;
    allOff();
    Serial.println("[SNOOZE] Snoozed " + String(PILL_NAME[activePill]) +
                   " for " + String(SNOOZE_DURATION_MIN) + " minutes.");
    notify(String(PILL_NAME[activePill]) + " snoozed for 10 minutes.");
    activePill = -1;
}

void checkSchedule() {
    DateTime now = rtc.now();
    for (int i = 0; i < PILL_COUNT; i++) {
        if (pillTaken[i] || pillMissed[i]) continue;

        int h = PILL_HOUR[i];
        int m = PILL_MINUTE[i] + snoozeExtra[i];
        h += m / 60;
        m  = m % 60;

        if (now.hour() == h && now.minute() == m) {
            triggerReminder(i);
            return;
        }
    }
}

void resetDaily() {
    Serial.println("[SYSTEM] New day – resetting states.");
    for (int i = 0; i < PILL_COUNT; i++) {
        pillTaken[i]  = false;
        pillMissed[i] = false;
        snoozeExtra[i]= 0;
    }
    reminderActive = false;
    activePill = -1;
    allOff();
}

// ── Setup ────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(200);

    pinMode(BUZZER,         OUTPUT);
    pinMode(LED_RED,        OUTPUT);
    pinMode(LED_YELLOW,     OUTPUT);
    pinMode(LED_GREEN,      OUTPUT);
    pinMode(PILL_SWITCH,    INPUT_PULLUP);
    pinMode(CONFIRM_BUTTON, INPUT_PULLUP);
    pinMode(SNOOZE_BUTTON,  INPUT_PULLUP);
    allOff();

    Wire.begin();
    if (!rtc.begin()) {
        Serial.println("[RTC] ERROR: not found!");
        while (1);
    }

    // Set RTC to 07:59:50 so the 08:00 morning reminder fires in ~10 seconds
    rtc.adjust(DateTime(2026, 5, 21, 7, 59, 50));

    Serial.println("========================================");
    Serial.println("  Smart Pill Reminder – SIMULATION");
    Serial.println("========================================");
    Serial.println("[INFO] Morning reminder fires in ~10 seconds.");
    Serial.println("[INFO] Press buttons on the diagram to interact:");
    Serial.println("       Blue   = Pill Box opened");
    Serial.println("       Green  = Confirm taken");
    Serial.println("       Yellow = Snooze 10 min");
    Serial.println("========================================\n");

    beep(2);
}

// ── Loop ─────────────────────────────────────────────────────
void loop() {
    DateTime now = rtc.now();

    if (now.day() != lastResetDay) {
        resetDaily();
        lastResetDay = now.day();
    }

    if (reminderActive) {
        // Buzzer beep + LED blink
        digitalWrite(BUZZER, HIGH);
        delay(BUZZER_ON_MS);
        digitalWrite(BUZZER, LOW);
        delay(BUZZER_OFF_MS);
        digitalWrite(PILL_LED[activePill],
                     !digitalRead(PILL_LED[activePill]));

        // Poll pill box switch
        if (digitalRead(PILL_SWITCH) == LOW) {
            delay(50);
            if (digitalRead(PILL_SWITCH) == LOW) {
                Serial.println("[EVENT] Pill box opened.");
                confirmPillTaken();
                return;
            }
        }

        // Poll confirm button
        if (digitalRead(CONFIRM_BUTTON) == LOW) {
            delay(50);
            if (digitalRead(CONFIRM_BUTTON) == LOW) {
                Serial.println("[EVENT] Confirm button pressed.");
                confirmPillTaken();
                return;
            }
        }

        // Poll snooze button
        if (digitalRead(SNOOZE_BUTTON) == LOW) {
            delay(50);
            if (digitalRead(SNOOZE_BUTTON) == LOW) {
                Serial.println("[EVENT] Snooze button pressed.");
                snoozeReminder();
                return;
            }
        }

        // Timeout check
        if (millis() - reminderStart >= REMINDER_WINDOW_MS) {
            pillMissed[activePill] = true;
            allOff();
            reminderActive = false;
            Serial.println("[ALERT] MISSED: " + String(PILL_NAME[activePill]));
            notify(String(PILL_NAME[activePill]) + " pill MISSED! Check on patient.");
            logCloud(activePill, 0);
            activePill = -1;
        }

    } else {
        checkSchedule();
    }

    delay(LOOP_DELAY_MS);
}
