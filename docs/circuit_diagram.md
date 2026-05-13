# Circuit Diagram – Smart Pill Reminder

## Component List

| # | Component              | Quantity | Notes                          |
|---|------------------------|----------|--------------------------------|
| 1 | NodeMCU ESP8266 v1.0   | 1        | Main microcontroller           |
| 2 | DS3231 RTC Module      | 1        | Real-time clock, I2C           |
| 3 | Active Buzzer 5V       | 1        | Audible alarm                  |
| 4 | LED Red                | 1        | Morning reminder indicator     |
| 5 | LED Yellow             | 1        | Afternoon reminder indicator   |
| 6 | LED Green              | 1        | Evening reminder indicator     |
| 7 | Resistor 220Ω          | 3        | Current limiting for LEDs      |
| 8 | Push Button (momentary)| 2        | Confirm + Snooze               |
| 9 | Reed Switch / Micro SW | 1        | Pill box lid detection         |
|10 | Breadboard             | 1        | Prototyping                    |
|11 | Jumper Wires           | ~20      | Male-to-male                   |
|12 | USB Cable (Micro-USB)  | 1        | Power + programming            |
|13 | 5V Power Supply        | 1        | Optional (USB power bank)      |

---

## Pin Connection Table

| NodeMCU Pin | GPIO | Connected To              | Notes                        |
|-------------|------|---------------------------|------------------------------|
| D1          | 5    | Buzzer (+)                | Buzzer (–) → GND             |
| D2          | 4    | LED Red (Anode via 220Ω)  | Cathode → GND                |
| D3          | 0    | LED Yellow (Anode via 220Ω)| Cathode → GND               |
| D4          | 2    | LED Green (Anode via 220Ω)| Cathode → GND                |
| D5          | 14   | Pill Switch (Pin 1)       | Pin 2 → GND (INPUT_PULLUP)   |
| D6          | 12   | Confirm Button (Pin 1)    | Pin 2 → GND (INPUT_PULLUP)   |
| D7          | 13   | Snooze Button (Pin 1)     | Pin 2 → GND (INPUT_PULLUP)   |
| D1 (SCL)    | 5    | DS3231 SCL                | I2C Clock                    |
| D2 (SDA)    | 4    | DS3231 SDA                | I2C Data                     |
| 3V3         | –    | DS3231 VCC                | 3.3V power                   |
| GND         | –    | DS3231 GND, all GNDs      | Common ground                |
| VIN / 5V    | –    | Buzzer VCC (if needed)    | 5V rail                      |

> **Note:** DS3231 uses I2C. On NodeMCU, I2C defaults to D1=SCL and D2=SDA.
> This means D1 and D2 serve dual purpose (LED + I2C). If you face conflicts,
> reassign LEDs to D8/D9 and update pin definitions in main.cpp.

---

## ASCII Wiring Diagram

```
                    NodeMCU ESP8266
                  ┌──────────────────┐
             RST ─┤                  ├─ A0
              EN ─┤                  ├─ D0 (GPIO16)
             3V3 ─┤──────────────────┼──► DS3231 VCC
             GND ─┤──────────────────┼──► DS3231 GND
                  │                  │    Buzzer GND
                  │                  │    All LED Cathodes
                  │                  │    Button Pin2 (×2)
                  │                  │    Switch Pin2
              CLK ─┤                  ├─ D1 (GPIO5)  ──► DS3231 SCL
              CMD ─┤                  ├─ D2 (GPIO4)  ──► DS3231 SDA
              SD3 ─┤                  ├─ D3 (GPIO0)  ──[220Ω]──► LED Yellow
              SD2 ─┤                  ├─ D4 (GPIO2)  ──[220Ω]──► LED Green
              SD1 ─┤                  ├─ D5 (GPIO14) ──► Pill Switch Pin1
              CMD ─┤                  ├─ D6 (GPIO12) ──► Confirm Button Pin1
              SD0 ─┤                  ├─ D7 (GPIO13) ──► Snooze Button Pin1
              CLK ─┤                  ├─ D8 (GPIO15)
              GND ─┤                  ├─ D9/RX
              VIN ─┤                  ├─ D10/TX
                  └──────────────────┘
                         │
                    D1 (GPIO5) ──► Buzzer (+)
                    D2 (GPIO4) ──[220Ω]──► LED Red
```

---

## DS3231 RTC Module Wiring

```
DS3231 Module
┌─────────────┐
│ VCC ────────┼──► NodeMCU 3V3
│ GND ────────┼──► NodeMCU GND
│ SDA ────────┼──► NodeMCU D2 (GPIO4)
│ SCL ────────┼──► NodeMCU D1 (GPIO5)
│ SQW         │   (not used)
│ 32K         │   (not used)
└─────────────┘
```

---

## LED Wiring (×3, same pattern)

```
NodeMCU D2/D3/D4
      │
    [220Ω]
      │
    ──┤►├──  LED (Anode → Cathode)
      │
     GND
```

---

## Button Wiring (×2, same pattern)

```
NodeMCU D6 or D7
      │
   [Button]
      │
     GND

(Internal pull-up enabled in code – no external resistor needed)
```

---

## Pill Box Switch Wiring

```
NodeMCU D5
      │
  [Reed/Micro Switch]
      │
     GND

(Closes circuit when pill box lid is opened)
```

---

## Power Supply Options

| Option          | Details                                      |
|-----------------|----------------------------------------------|
| USB from PC     | For development and testing                  |
| USB Power Bank  | Portable, 5V 1A minimum recommended          |
| 5V Wall Adapter | Permanent installation, micro-USB connector  |

---

## Breadboard Layout (Top View)

```
+──────────────────────────────────────────────────────+
│  BREADBOARD                                          │
│                                                      │
│  [NodeMCU]  [DS3231]  [BUZ] [R][Y][G]  [BTN1][BTN2] │
│     │           │       │    │  │  │      │     │    │
│  ───┴───────────┴───────┴────┴──┴──┴──────┴─────┴──  │
│  GND rail ─────────────────────────────────────────  │
│  VCC rail ─────────────────────────────────────────  │
+──────────────────────────────────────────────────────+
```
