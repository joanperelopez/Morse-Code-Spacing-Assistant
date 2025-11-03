# Morse-Code-Spacing-Assistant

## 1. Overview
The **CW Visual Metronome** is a training device designed for radio amateurs practicing Morse code (CW).  
It helps the operator maintain correct **Farnsworth timing** between characters and words by providing **visual cues** through two LEDs.

The device monitors the keyer signal, detects symbol endings, and activates visual indicators at precise intervals derived from the selected **effective WPM** (Words Per Minute).

The system is based on an **Arduino Nano**, with a **1602 LCD display** showing the current speed, a **rotary encoder** for adjustment, and a **memory pushbutton** that stores the selected speed in non-volatile EEPROM memory. 

---

## 2. Functional Description

| Function | Description |
|-----------|--------------|
| **Keyer input** | Detects the Morse key line. When released, timing intervals begin. |
| **Character LED** | Lights up when the inter-character interval (3 dot units) has elapsed since the last key release. |
| **Word LED** | Lights up when the inter-word interval (7 dot units) has elapsed since the last key release. |
| **Rotary encoder** | Adjusts the effective Farnsworth speed (WPM). |
| **LCD display** | Shows the current Farnsworth speed in WPM. |
| **Memory button** | When pressed (active LOW), stores the current speed in EEPROM. |
| **EEPROM memory** | Retains the last stored speed; restored automatically at startup. |

---

## 3. Timing Definition (Farnsworth Method)

The Farnsworth method keeps the character speed constant but lengthens the spacing between characters and words to achieve a slower **effective WPM** for training.

The base time unit (*dot duration*) is calculated as:




| Interval | Duration (dot units) | Description |
|-----------|----------------------|--------------|
| Dot (·) | 1 × Tdot | Base unit |
| Dash (–) | 3 × Tdot | Morse standard |
| Intra-character gap | 1 × Tdot | Between elements of a character |
| Inter-character gap | 3 × Tdot | Between two characters |
| Inter-word gap | 7 × Tdot | Between words |

LED indicators:
- **Character LED:** lights after *3 × Tdot* (character spacing).  
- **Word LED:** lights after *7 × Tdot* (word spacing).

---

## 4. User Interface and Operation

1. **Startup**
   - Reads stored speed from EEPROM.
   - Defaults to **15 WPM** if no valid data found.
   - Displays current speed on the LCD.

2. **Adjusting the Speed**
   - Turn the rotary encoder to change the Farnsworth speed.
   - Display updates in real time.

3. **Saving the Speed**
   - Press the *Memory Button* (active LOW) to save current WPM to EEPROM.
   - “Saved in EEPROM” appears briefly on the LCD.

4. **Operating Mode**
   - When the Morse key (active LOW) is pressed, both LEDs turn OFF.
   - After key release:
     - The **Character LED** turns ON after 3 dot units.
     - The **Word LED** turns ON after 7 dot units.
   - When the key is pressed again, both LEDs turn OFF immediately.

---

## 5. Hardware Specifications

| Component | Description |
|------------|--------------|
| **Microcontroller** | Arduino Nano (ATmega328P) |
| **Display** | 1602 alphanumeric LCD |
| **Rotary Encoder** | Incremental (2-channel, mechanical) |
| **EEPROM** | Internal, used for WPM storage |
| **Indicators** | 2 LEDs (character and word) |
| **Memory Button** | Momentary pushbutton, active LOW |
| **Keyer Input** | From key or transceiver, active LOW |
| **Power Supply** | 5 V DC (USB or external) |

---

## 6. Pin Assignments

| Signal | Arduino Pin | Description |
|---------|--------------|-------------|
| **Keyer input** | D2 | Active LOW, internal pull-up |
| **Encoder A** | D3 | Rotary encoder channel A |
| **Encoder B** | D4 | Rotary encoder channel B |
| **LCD D4** | D5 | Data line 4 |
| **LCD D5** | D6 | Data line 5 |
| **LCD RS** | D7 | Register Select |
| **LCD E** | D8 | Enable |
| **LED (character)** | D9 | Character spacing indicator |
| **LED (word)** | D10 | Word spacing indicator |
| **LCD D6** | D11 | Data line 6 |
| **LCD D7** | D12 | Data line 7 |
| **Memory button** | D21 | Active LOW, internal pull-up |

---

## 7. Electrical Characteristics

| Parameter | Typical Value | Notes |
|------------|----------------|-------|
| Supply Voltage | 5 V DC | Standard Arduino Nano |
| Logic HIGH | ≥ 3.0 V | TTL compatible |
| Logic LOW | ≤ 1.0 V | TTL compatible |
| LED Current | 10–15 mA | With 330 Ω series resistors |
| EEPROM Endurance | 100,000 write cycles | Managed via `EEPROM.update()` |

---

## 8. Software Summary

- Language: **C++ (Arduino IDE)**
- Main libraries used:
  - `LiquidCrystal.h` – LCD display control  
  - `RotaryEncoder.h` – rotary encoder handling  
  - `EEPROM.h` – non-volatile memory management
- Main functions:
  - Real-time LED timing based on keyer signal
  - Adjustable Farnsworth WPM
  - EEPROM storage of configuration
  - LCD visual feedback

---

## 9. Default Parameters

| Parameter | Default Value | Range |
|------------|----------------|--------|
| **Farnsworth speed** | 15 WPM | 5–40 WPM |
| **Character spacing** | 3 × Tdot | — |
| **Word spacing** | 7 × Tdot | — |

---

## 10. Author
Developed by **Pere López Veraguas (EA3AGK)**  
Barcelona, Spain — 2025

---



<img width="1271" height="861" alt="Metronomo_TOP" src="https://github.com/user-attachments/assets/cfeb9a91-e12c-4f3b-a6e2-9ec90241dd29" />

<img width="1271" height="861" alt="Metronomo_BOT" src="https://github.com/user-attachments/assets/445ec403-39f9-428a-92fc-53695e28b293" />
