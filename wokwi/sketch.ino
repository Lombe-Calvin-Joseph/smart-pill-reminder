/*
 * ============================================================
 *  Smart Pill Reminder – WOKWI SIMULATION (Arduino Uno)
 *  No WiFi/Blynk/ThingSpeak – pure logic simulation
 *  RTC starts at 07:59:50 → morning reminder fires in ~10s
 * ============================================================
 */

#include <Wire.h>
#include <RTClib.h>

// ── Pins (Arduino Uno) ───────────────────────────────────────
#define BUZZER          5   // D5
#define LED_RED         4   // D4 – Morning
#define LED_YELLOW      3   // D3 – Afternoon
#define LED_GREEN       2   // D2 – Evening
#define PILL_SWITCH     8   // D8 – Pill box lid (blue button)
#define CONFIRM_BUTTON  7   // D7 – Confirm taken (green button)
#define SNOOZE_BUTTON   6   // D6 – Snooze (yellow button)

// ── Schedule ─────────────────────────────────────────────────
const int PILL_COUNT = 3;
const int PILL_HOUR[PILL_COUNT]   = {  8, 13, 20 };
const int PILL_MINUTE[PILL_COUNT] = {  0,  0,  0 };
const char* PILL_NAME[]           = { "Morning", "Afternoon", "Evening" };
const int   PILL_LED[]            = { LED_RED, LED_YELLOW, LED_GREEN };

#define REMINDER_WINDOW_MS  (5UL * 60 * 1000)
#define SNOOZE_MINUTES      10
#define MAX_SNOOZE_COUNT     3
#define BUZZER_ON_MS        400
#define BUZZER_OFF_MS       400

// ── State ────────────────────────────────────────────────────
RTC_DS3231 rtc;

bool pillTaken[PILL_COUNT]   = { false, false, false };
bool pillMissed[PILL_COUNT]  = { false, false, false };
int  snoozeExtra[PILL_COUNT] = { 0, 0, 0 };
int  snoozeCount[PILL_COUNT] = { 0, 0, 0 };

bool          reminderActive = false;
int           activePill     = -1;
unsigned long reminderStart  = 0;
int           lastResetDay   = -1;

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

// ── Core ─────────────────────────────────────────────────────
void triggerReminder(int idx) {
    activePill     = idx;
    reminderActive = true;
    reminderStart  = millis();
    digitalWrite(PILL_LED[idx], HIGH);

    Serial.println();
    Serial.print(F("[REMINDER] >>> "));
    Serial.print(PILL_NAME[idx]);
    Serial.println(F(" pill time! <<<"));
    Serial.print(F("[INFO] Snoozes left: "));
    Serial.println(MAX_SNOOZE_COUNT - snoozeCount[idx]);
}

void confirmPillTaken() {
    pillTaken[activePill] = true;
    reminderActive = false;
    allOff();
    beep(3);

    Serial.print(F("[CONFIRM] "));
    Serial.print(PILL_NAME[activePill]);
    Serial.println(F(" pill TAKEN. Good job!"));
    activePill = -1;
}

void snoozeReminder() {
    if (snoozeCount[activePill] >= MAX_SNOOZE_COUNT) {
        Serial.println(F("[SNOOZE] Max snoozes reached – marking as missed."));
        pillMissed[activePill] = true;
        allOff();
        reminderActive = false;
        Serial.print(F("[ALERT]  MISSED: "));
        Serial.println(PILL_NAME[activePill]);
        activePill = -1;
        return;
    }

    snoozeCount[activePill]++;
    snoozeExtra[activePill] += SNOOZE_MINUTES;
    reminderActive = false;
    allOff();

    Serial.print(F("[SNOOZE]  "));
    Serial.print(PILL_NAME[activePill]);
    Serial.print(F(" snoozed. ("));
    Serial.print(MAX_SNOOZE_COUNT - snoozeCount[activePill]);
    Serial.println(F(" left)"));
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
    Serial.println(F("[SYSTEM] New day – resetting states."));
    for (int i = 0; i < PILL_COUNT; i++) {
        pillTaken[i]  = false;
        pillMissed[i] = false;
        snoozeExtra[i]= 0;
        snoozeCount[i]= 0;
    }
    reminderActive = false;
    activePill = -1;
    allOff();
}

// ── Setup ────────────────────────────────────────────────────
void setup() {
    Serial.begin(9600);
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
        Serial.println(F("[RTC] ERROR: DS3231 not found!"));
        while (1);
    }

    // Set time to 07:59:50 → morning reminder fires in ~10 seconds
    rtc.adjust(DateTime(2026, 5, 21, 7, 59, 50));

    Serial.println(F("========================================"));
    Serial.println(F("  Smart Pill Reminder - SIMULATION"));
    Serial.println(F("  Board: Arduino Uno"));
    Serial.println(F("========================================"));
    Serial.println(F("[INFO] Morning reminder fires in ~10 sec"));
    Serial.println(F("[INFO] Buttons:"));
    Serial.println(F("       BLUE   = Pill Box opened"));
    Serial.println(F("       GREEN  = Confirm taken"));
    Serial.println(F("       YELLOW = Snooze (max 3x)"));
    Serial.println(F("========================================"));

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
        digitalWrite(BUZZER, HIGH);
        delay(BUZZER_ON_MS);
        digitalWrite(BUZZER, LOW);
        delay(BUZZER_OFF_MS);
        digitalWrite(PILL_LED[activePill],
                     !digitalRead(PILL_LED[activePill]));

        if (digitalRead(PILL_SWITCH) == LOW) {
            delay(50);
            if (digitalRead(PILL_SWITCH) == LOW) {
                Serial.println(F("[EVENT] Pill box opened."));
                confirmPillTaken();
                return;
            }
        }
        if (digitalRead(CONFIRM_BUTTON) == LOW) {
            delay(50);
            if (digitalRead(CONFIRM_BUTTON) == LOW) {
                Serial.println(F("[EVENT] Confirm button pressed."));
                confirmPillTaken();
                return;
            }
        }
        if (digitalRead(SNOOZE_BUTTON) == LOW) {
            delay(50);
            if (digitalRead(SNOOZE_BUTTON) == LOW) {
                Serial.println(F("[EVENT] Snooze button pressed."));
                snoozeReminder();
                return;
            }
        }

        if (millis() - reminderStart >= REMINDER_WINDOW_MS) {
            pillMissed[activePill] = true;
            allOff();
            reminderActive = false;
            Serial.print(F("[ALERT]  MISSED: "));
            Serial.println(PILL_NAME[activePill]);
            activePill = -1;
        }

    } else {
        checkSchedule();
        delay(300);
    }
}
