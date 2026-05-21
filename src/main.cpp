/*
 * ============================================================
 *  Smart Pill Reminder and Medicine Tracker
 *  Author  : [Your Name]
 *  Date    : 2026
 *  Board   : Arduino Uno
 *  Purpose : Remind patients to take medication at scheduled
 *            times, detect pill box opening, and log events
 *            to Serial for monitoring.
 * ============================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

// ─────────────────────────────────────────────
//  PIN DEFINITIONS  (Arduino Uno)
//  I2C (DS3231): A4 = SDA, A5 = SCL  (hardware fixed)
// ─────────────────────────────────────────────
#define BUZZER          5   // D5 – active buzzer
#define LED_RED         4   // D4 – morning reminder
#define LED_YELLOW      3   // D3 – afternoon reminder
#define LED_GREEN       2   // D2 – evening reminder
#define PILL_SWITCH     8   // D8 – reed/micro-switch on pill box lid
#define CONFIRM_BUTTON  7   // D7 – manual "I took my pill" button
#define SNOOZE_BUTTON   6   // D6 – snooze 10 minutes

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
#define REMINDER_WINDOW_MS  (5UL * 60 * 1000)  // 5 minutes
#define SNOOZE_DURATION_MIN  10                 // snooze delay in minutes
#define MAX_SNOOZE_COUNT      3                 // max snoozes per dose
#define BUZZER_ON_MS        400
#define BUZZER_OFF_MS       400

// ─────────────────────────────────────────────
//  GLOBAL OBJECTS
// ─────────────────────────────────────────────
RTC_DS3231 rtc;

// ─────────────────────────────────────────────
//  STATE VARIABLES
// ─────────────────────────────────────────────
bool          pillTaken[PILL_COUNT]       = { false, false, false };
bool          pillMissed[PILL_COUNT]      = { false, false, false };
bool          reminderActive              = false;
int           activePillIndex             = -1;
unsigned long reminderStartMillis         = 0;
int           snoozeExtraMinutes[PILL_COUNT] = { 0, 0, 0 };
int           snoozeCount[PILL_COUNT]     = { 0, 0, 0 };
int           lastResetDay                = -1;

// ─────────────────────────────────────────────
//  FORWARD DECLARATIONS
// ─────────────────────────────────────────────
void initRTC();
void checkSchedule();
void triggerReminder(int pillIndex);
void confirmPillTaken();
void snoozeReminder();
void resetDailyState();
void printTime();
void buzzerBeep(int times);
void allLEDsOff();
void allOutputsOff();

// ─────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────
void setup() {
    Serial.begin(9600);
    delay(200);
    Serial.println(F("========================================"));
    Serial.println(F("  Smart Pill Reminder - Booting..."));
    Serial.println(F("  Board: Arduino Uno"));
    Serial.println(F("========================================"));

    pinMode(BUZZER,         OUTPUT);
    pinMode(LED_RED,        OUTPUT);
    pinMode(LED_YELLOW,     OUTPUT);
    pinMode(LED_GREEN,      OUTPUT);
    pinMode(PILL_SWITCH,    INPUT_PULLUP);
    pinMode(CONFIRM_BUTTON, INPUT_PULLUP);
    pinMode(SNOOZE_BUTTON,  INPUT_PULLUP);

    allOutputsOff();
    initRTC();
    buzzerBeep(2);

    Serial.println(F("[SYSTEM] Startup complete."));
    Serial.print(F("[SYSTEM] Current time: "));
    printTime();
    Serial.println(F("========================================"));
}

// ─────────────────────────────────────────────
//  MAIN LOOP
// ─────────────────────────────────────────────
void loop() {
    DateTime now = rtc.now();

    // Reset pill states at midnight each new day
    if (now.day() != lastResetDay) {
        resetDailyState();
        lastResetDay = now.day();
    }

    if (reminderActive) {
        // Buzz and blink while waiting for confirmation
        digitalWrite(BUZZER, HIGH);
        delay(BUZZER_ON_MS);
        digitalWrite(BUZZER, LOW);
        delay(BUZZER_OFF_MS);
        digitalWrite(PILL_LED[activePillIndex],
                     !digitalRead(PILL_LED[activePillIndex]));

        // Pill box switch – physical confirmation
        if (digitalRead(PILL_SWITCH) == LOW) {
            delay(50);
            if (digitalRead(PILL_SWITCH) == LOW) {
                Serial.println(F("[EVENT] Pill box opened."));
                confirmPillTaken();
                return;
            }
        }

        // Confirm button
        if (digitalRead(CONFIRM_BUTTON) == LOW) {
            delay(50);
            if (digitalRead(CONFIRM_BUTTON) == LOW) {
                Serial.println(F("[EVENT] Confirm button pressed."));
                confirmPillTaken();
                return;
            }
        }

        // Snooze button
        if (digitalRead(SNOOZE_BUTTON) == LOW) {
            delay(50);
            if (digitalRead(SNOOZE_BUTTON) == LOW) {
                Serial.println(F("[EVENT] Snooze button pressed."));
                snoozeReminder();
                return;
            }
        }

        // Timeout – pill was missed
        if (millis() - reminderStartMillis >= REMINDER_WINDOW_MS) {
            pillMissed[activePillIndex] = true;
            allOutputsOff();
            reminderActive = false;

            Serial.print(F("[ALERT]  MISSED: "));
            Serial.print(PILL_NAME[activePillIndex]);
            Serial.println(F(" pill! Please check on patient."));

            activePillIndex = -1;
        }

    } else {
        checkSchedule();
        delay(300);
    }
}

// ─────────────────────────────────────────────
//  RTC INITIALIZATION
// ─────────────────────────────────────────────
void initRTC() {
    Wire.begin();
    if (!rtc.begin()) {
        Serial.println(F("[RTC] ERROR: DS3231 not detected! Check wiring."));
        // Blink all LEDs rapidly to signal hardware error
        for (int i = 0; i < 10; i++) {
            digitalWrite(LED_RED,    HIGH);
            digitalWrite(LED_YELLOW, HIGH);
            digitalWrite(LED_GREEN,  HIGH);
            delay(300);
            allLEDsOff();
            delay(300);
        }
        while (1); // halt – cannot operate without RTC
    }

    if (rtc.lostPower()) {
        Serial.println(F("[RTC] WARNING: RTC lost power – setting compile time."));
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    Serial.println(F("[RTC] DS3231 initialized OK."));
}

// ─────────────────────────────────────────────
//  CHECK SCHEDULE
// ─────────────────────────────────────────────
void checkSchedule() {
    DateTime now = rtc.now();
    int h = now.hour();
    int m = now.minute();

    for (int i = 0; i < PILL_COUNT; i++) {
        if (pillTaken[i] || pillMissed[i]) continue;

        int schedH = PILL_HOUR[i];
        int schedM = PILL_MINUTE[i] + snoozeExtraMinutes[i];
        schedH += schedM / 60;
        schedM  = schedM % 60;

        if (h == schedH && m == schedM) {
            triggerReminder(i);
            return; // handle one at a time
        }
    }
}

// ─────────────────────────────────────────────
//  TRIGGER REMINDER
// ─────────────────────────────────────────────
void triggerReminder(int pillIndex) {
    activePillIndex     = pillIndex;
    reminderActive      = true;
    reminderStartMillis = millis();

    digitalWrite(PILL_LED[pillIndex], HIGH);

    Serial.println();
    Serial.print(F("[REMINDER] >>> "));
    Serial.print(PILL_NAME[pillIndex]);
    Serial.println(F(" pill time! <<<"));
    Serial.print(F("[INFO] Waiting up to 5 min. Snoozes left: "));
    Serial.println(MAX_SNOOZE_COUNT - snoozeCount[pillIndex]);
}

// ─────────────────────────────────────────────
//  CONFIRM PILL TAKEN
// ─────────────────────────────────────────────
void confirmPillTaken() {
    if (activePillIndex < 0) return;

    pillTaken[activePillIndex] = true;
    reminderActive = false;
    allOutputsOff();
    buzzerBeep(3);

    Serial.print(F("[CONFIRM] "));
    Serial.print(PILL_NAME[activePillIndex]);
    Serial.println(F(" pill TAKEN. Good job!"));

    activePillIndex = -1;
}

// ─────────────────────────────────────────────
//  SNOOZE REMINDER
// ─────────────────────────────────────────────
void snoozeReminder() {
    if (activePillIndex < 0) return;

    // Enforce snooze limit
    if (snoozeCount[activePillIndex] >= MAX_SNOOZE_COUNT) {
        Serial.println(F("[SNOOZE] Max snoozes reached – marking as missed."));
        pillMissed[activePillIndex] = true;
        allOutputsOff();
        reminderActive = false;
        Serial.print(F("[ALERT]  MISSED: "));
        Serial.println(PILL_NAME[activePillIndex]);
        activePillIndex = -1;
        return;
    }

    snoozeCount[activePillIndex]++;
    snoozeExtraMinutes[activePillIndex] += SNOOZE_DURATION_MIN;
    reminderActive = false;
    allOutputsOff();

    Serial.print(F("[SNOOZE]  "));
    Serial.print(PILL_NAME[activePillIndex]);
    Serial.print(F(" snoozed for "));
    Serial.print(SNOOZE_DURATION_MIN);
    Serial.print(F(" min. ("));
    Serial.print(MAX_SNOOZE_COUNT - snoozeCount[activePillIndex]);
    Serial.println(F(" snoozes left)"));

    activePillIndex = -1;
}

// ─────────────────────────────────────────────
//  RESET DAILY STATE  (called at midnight)
// ─────────────────────────────────────────────
void resetDailyState() {
    Serial.println(F("[SYSTEM] New day – resetting pill states."));
    for (int i = 0; i < PILL_COUNT; i++) {
        pillTaken[i]          = false;
        pillMissed[i]         = false;
        snoozeExtraMinutes[i] = 0;
        snoozeCount[i]        = 0;
    }
    reminderActive  = false;
    activePillIndex = -1;
    allOutputsOff();
}

// ─────────────────────────────────────────────
//  PRINT CURRENT TIME TO SERIAL
// ─────────────────────────────────────────────
void printTime() {
    DateTime now = rtc.now();
    char buf[25];
    snprintf(buf, sizeof(buf),
             "%02d:%02d:%02d %02d/%02d/%04d",
             now.hour(), now.minute(), now.second(),
             now.day(),  now.month(),  now.year());
    Serial.println(buf);
}

// ─────────────────────────────────────────────
//  HELPERS
// ─────────────────────────────────────────────
void buzzerBeep(int times) {
    for (int i = 0; i < times; i++) {
        digitalWrite(BUZZER, HIGH); delay(200);
        digitalWrite(BUZZER, LOW);  delay(150);
    }
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
