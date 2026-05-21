# Circuit Diagram – Smart Pill Reminder

## Component List

| # | Component              | Quantity | Notes                          |
|---|------------------------|----------|--------------------------------|
| 1 | Arduino Uno R3         | 1        | Main microcontroller           |
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
|12 | USB Cable (Type-B)     | 1        | Power + programming            |

---

## Pin Connection Table

| Arduino Pin | Connected To               | Notes                        |
|-------------|----------------------------|------------------------------|
| D2          | LED Green (Anode via 220Ω) | Cathode → GND                |
| D3          | LED Yellow (Anode via 220Ω)| Cathode → GND                |
| D4          | LED Red (Anode via 220Ω)   | Cathode → GND                |
| D5          | Buzzer (+)                 | Buzzer (–) → GND             |
| D6          | Snooze Button (Pin 1)      | Pin 2 → GND (INPUT_PULLUP)   |
| D7          | Confirm Button (Pin 1)     | Pin 2 → GND (INPUT_PULLUP)   |
| D8          | Pill Switch (Pin 1)        | Pin 2 → GND (INPUT_PULLUP)   |
| A4 (SDA)    | DS3231 SDA                 | I2C Data                     |
| A5 (SCL)    | DS3231 SCL                 | I2C Clock                    |
| 5V          | DS3231 VCC                 | 5V power                     |
| GND         | DS3231 GND, all GNDs       | Common ground                |

---

## ASCII Wiring Diagram

```
                    Arduino Uno
                  ┌──────────────────┐
              5V ─┤──────────────────┼──► DS3231 VCC
             GND ─┤──────────────────┼──► DS3231 GND
                  │                  │    Buzzer GND
                  │                  │    All LED Cathodes
                  │                  │    Button/Switch Pin2
              A4 ─┤ (SDA)            ├──► DS3231 SDA
              A5 ─┤ (SCL)            ├──► DS3231 SCL
              D2 ─┤                  ├──[220Ω]──► LED Green  (Evening)
              D3 ─┤                  ├──[220Ω]──► LED Yellow (Afternoon)
              D4 ─┤                  ├──[220Ω]──► LED Red    (Morning)
              D5 ─┤                  ├──► Buzzer (+)
              D6 ─┤                  ├──► Snooze Button Pin1
              D7 ─┤                  ├──► Confirm Button Pin1
              D8 ─┤                  ├──► Pill Switch Pin1
                  └──────────────────┘
```

---

## DS3231 RTC Module Wiring

```
DS3231 Module
┌─────────────┐
│ VCC ────────┼──► Arduino 5V
│ GND ────────┼──► Arduino GND
│ SDA ────────┼──► Arduino A4
│ SCL ────────┼──► Arduino A5
│ SQW         │   (not used)
│ 32K         │   (not used)
└─────────────┘
```

---

## LED Wiring (×3, same pattern)

```
Arduino D2 / D3 / D4
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
Arduino D6 or D7
      │
   [Button]
      │
     GND

(Internal pull-up enabled – no external resistor needed)
```

---

## Pill Box Switch Wiring

```
Arduino D8
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
| 9V Wall Adapter | Permanent installation via barrel jack       |
