/*
  Version 2.2
  CW Visual and audio Metronome - Edit CHAR and FARM speeds
  Arduino Nano, PCB pinout as specified by Pere (EA3AGK)

  - Keyer input (active LOW)         : D2
  - Rotary encoder A/B               : D3, D4
  - LCD 1602 (4-bit)                 : D5 (D4), D6 (D5), D7 (RS), D8 (E), D11 (D6), D12 (D7)
  - LED character (char gap)         : D10
  - LED word (word gap)              : D9
  - Option button (active LOW)       : A2  (used with internal pull-up)
  - EEPROM:
      addr 0 -> wpm_char (5..35) default 25
      addr 1 -> wpm_eff  (5..35) default 6

  Behaviour implemented:
  - Normal operation as before (keyer -> LEDs based on Farnsworth spacing)
  - Press Option to enter edit CHAR mode (numeric blinks). Change with encoder (5..35).
    Timeout: 3 s of encoder inactivity -> save to EEPROM and exit edit.
    While moving the encoder the timeout is suspended (i.e. reset).
  - Press Option again while in CHAR edit -> switch to FARM edit (same behaviour).
  - If editing CHAR and new CHAR < current FARM, FARM is capped to CHAR when saving.
  - At startup read EEPROM values; if invalid use defaults (25, 6).
  - dot timing computed from wpm_char; spacing scaled so that effective spacing obeys wpm_eff:
      dot_ms = 1200 / wpm_char
      scale = wpm_char / wpm_eff  (>= 1)
      charGap = 3 * dot_ms * scale
      wordGap = 7 * dot_ms * scale

  Requires libraries: LiquidCrystal, RotaryEncoder, EEPROM
*/



#include <LiquidCrystal.h>
#include <RotaryEncoder.h>
#include <EEPROM.h>

// ---------------- PINES
const uint8_t PIN_KEYER    = 2;   // active LOW
const uint8_t PIN_ROT_A    = 3;
const uint8_t PIN_ROT_B    = 4;
const uint8_t PIN_LCD_D4   = 5;
const uint8_t PIN_LCD_D5   = 6;
const uint8_t PIN_LCD_RS   = 7;
const uint8_t PIN_LCD_E    = 8;
const uint8_t PIN_LED_CHAR = 10;
const uint8_t PIN_LED_WORD = 9;
const uint8_t PIN_LCD_D6   = 11;
const uint8_t PIN_LCD_D7   = 12;
const uint8_t PIN_OPTION   = A1;  // active LOW, internal pull-up
const uint8_t pinBuzzer    = 13;   

// ---------------- HARDWARE OBJECTS
LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_E,
                  PIN_LCD_D4, PIN_LCD_D5,
                  PIN_LCD_D6, PIN_LCD_D7);

RotaryEncoder encoder(PIN_ROT_A, PIN_ROT_B);

// Nuevas variables para el calculo del Farnsworth
float t_dot;       // duración de un dit según wpm_char
float t_dot_eff;   // duración de un dit según wpm_eff
float t_info;      // tiempo total del mensaje Morse, sin Farnsworth
float t_totalSlots; 
float t_slot;      
float n_slots;     

float t_char_sec;  // segundos entre caracteres
float t_word_sec;  // segundos entre palabras

// ---------------- EEPROM & defaults
const uint8_t ADDR_CHAR = 0;
const uint8_t ADDR_EFF  = 1;

const int DEF_CHAR = 25;
const int DEF_EFF  = 6;
const int MIN_WPM = 3;
const int MAX_WPM = 40;

// ---------------- ESTADO
int wpm_char = DEF_CHAR;
int wpm_eff  = DEF_EFF;

unsigned long charGap_ms = 0;
unsigned long wordGap_ms = 0;

unsigned long lastKeyRelease_ms = 0;
bool lastKeyState = HIGH;
bool keyState;


enum Mode { MODE_NORMAL, MODE_EDIT_CHAR, MODE_EDIT_FARM };
Mode modeNow = MODE_NORMAL;

int editValue = 0;

// encoder pos tracking
long encLastPos = 0;

// edit timing & blink
unsigned long lastEncoderActivity = 0;
const unsigned long EDIT_TIMEOUT = 3000UL; // 3 s
unsigned long lastBlink = 0;
const unsigned long BLINK_MS = 500UL;
bool blinkOn = true;
bool valueMoved = false;

// option button debounce
bool optLastRaw = HIGH;
unsigned long optDebounceTime = 0;
const unsigned long OPT_DEBOUNCE_MS = 40;
static bool optLastStable = HIGH;


// Timer LEDs

unsigned long startTime = 0;
bool waitingToTurnOn_C = false;
bool waitingToTurnOn_W = false;
bool buttonWasPressed = false;



// ---------------- FUNCIONES AUXILIARES

// Funcion de calculo del Farnsworth
void computeFarnsworthTiming(int wpm_char, int wpm_eff) {
  t_dot = 60.0 / (wpm_char * 50.0);
  t_dot_eff = 60.0 / (wpm_eff * 50.0);

  t_info = t_dot * (50 - 19) * wpm_eff;

  t_totalSlots = 60.0 - t_info;

  n_slots = 3.0 * ((wpm_eff * 5) - wpm_eff) + 7.0 * wpm_eff;

  t_slot = t_totalSlots / n_slots;

  t_char_sec = 3.0 * t_slot*1.1;          // Para cálculo de tiempo en segundos
  t_word_sec = 7.0 * t_slot*1.1;          // Para calculo de tiempo en segundos

  charGap_ms = (unsigned long)(t_char_sec * 1000.0 + 0.5);
  wordGap_ms = (unsigned long)(t_word_sec * 1000.0 + 0.5);
}

//*******************************************
// Mostrar los tiempos en el LCD
void showLEDTimes() {
  lcd.setCursor(13,0);
  lcd.print("    ");
  lcd.setCursor(13,0);
  lcd.print(t_char_sec,2);

  lcd.setCursor(13,1);
  lcd.print("    ");
  lcd.setCursor(13,1);
  lcd.print(t_word_sec,2);
}

//*******************************************
// Lectura NVM
int safeReadEEP(int addr, int defv) {
  int v = EEPROM.read(addr);
  if (v < MIN_WPM || v > MAX_WPM) return defv;
  return v;
}
// Lectura NVM
void loadEEP() {
  wpm_char = safeReadEEP(ADDR_CHAR, DEF_CHAR);
  wpm_eff  = safeReadEEP(ADDR_EFF, DEF_EFF);
  if (wpm_eff > wpm_char) wpm_eff = wpm_char;
  computeFarnsworthTiming(wpm_char, wpm_eff);
}
// Guardar en NVM velocidad de caracter
void saveCharEEP() {
  EEPROM.update(ADDR_CHAR, (uint8_t)wpm_char);
}
// Guardar en NVM velocidad Farnsworth
void saveEffEEP() {
  EEPROM.update(ADDR_EFF, (uint8_t)wpm_eff);
}

// pantallas
//Vista normal del display
void showNormal() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("CHAR:");
  lcd.setCursor(6,0);
  if (wpm_char < 10) lcd.print('0');
  lcd.print(wpm_char);
  lcd.print(" wpm");

  lcd.setCursor(0,1);
  lcd.print("FARM:");
  lcd.setCursor(6,1);
  if (wpm_eff < 10) lcd.print('0');
  lcd.print(wpm_eff);
  lcd.print(" wpm");

  showLEDTimes();       // Se muestran los tiempos en segundos
}

// Pantalla de edición
// Display de edición
void showEditHeader(Mode m) {
  lcd.clear();
  if (m == MODE_EDIT_CHAR) {
    lcd.setCursor(0,0); lcd.print("CHAR:");
  } else {
    lcd.setCursor(0,0); lcd.print("FARM:");
  }
  lcd.setCursor(11,0); lcd.print(" wpm");
}

void updateEditNumber(Mode m, int value, bool showNumber) {
  lcd.setCursor(6,0);
  if (showNumber) {
    if (value < 10) lcd.print('0');
    lcd.print(value);
  } else {
    lcd.print("  ");
  }
}

// option button detection
bool optionPressed() {
  bool raw = digitalRead(PIN_OPTION);
  if (raw != optLastRaw) {
    optDebounceTime = millis();
    optLastRaw = raw;
  }
  if ((millis() - optDebounceTime) > OPT_DEBOUNCE_MS) {
    if (optLastStable == HIGH && raw == LOW) {
      optLastStable = raw;
      return true;
    }
    optLastStable = raw;
  }
  return false;
}

//****************************************
// ---------------- SETUP
void setup() {
  pinMode(PIN_KEYER, INPUT_PULLUP);
  pinMode(PIN_OPTION, INPUT_PULLUP);
  pinMode(PIN_LED_CHAR, OUTPUT);
  pinMode(PIN_LED_WORD, OUTPUT);

  digitalWrite(PIN_LED_CHAR, LOW);
  digitalWrite(PIN_LED_WORD, LOW);

  waitingToTurnOn_C = true;     // Temporizador
  waitingToTurnOn_W = true;

  Serial.begin(115200);   
  lcd.begin(16,2);

  loadEEP();
  showNormal();

  encoder.setPosition(0);
  encLastPos = encoder.getPosition();
  lastEncoderActivity = millis();
  lastBlink = millis();

  lastKeyState = digitalRead(PIN_KEYER);
  lastKeyRelease_ms = millis();
  lastKeyRelease_ms = 0;
}

//****************************************
// ---------------- LOOP
void loop() {
  encoder.tick();
  long encPos = encoder.getPosition();
  long delta = encPos - encLastPos;


  // OPTION button
  if (optionPressed()) {
    if (modeNow == MODE_NORMAL) {
      modeNow = MODE_EDIT_CHAR;
      editValue = wpm_char;
      encoder.setPosition(0); encLastPos = 0;
      lastEncoderActivity = millis();
      blinkOn = true; lastBlink = millis();
      showEditHeader(MODE_EDIT_CHAR);
      updateEditNumber(MODE_EDIT_CHAR, editValue, blinkOn);
      valueMoved = false;
    } else if (modeNow == MODE_EDIT_CHAR) {
      modeNow = MODE_EDIT_FARM;
      editValue = wpm_eff;
      encoder.setPosition(0); encLastPos = 0;
      lastEncoderActivity = millis();
      blinkOn = true; lastBlink = millis();
      showEditHeader(MODE_EDIT_FARM);
      updateEditNumber(MODE_EDIT_FARM, editValue, blinkOn);
      valueMoved = false;
    } else if (modeNow == MODE_EDIT_FARM) {
      modeNow = MODE_EDIT_CHAR;
      editValue = wpm_char;
      encoder.setPosition(0); encLastPos = 0;
      lastEncoderActivity = millis();
      blinkOn = true; lastBlink = millis();
      showEditHeader(MODE_EDIT_CHAR);
      updateEditNumber(MODE_EDIT_CHAR, editValue, blinkOn);
      valueMoved = false;
    }
  }

  // --- Edición
  if (modeNow == MODE_EDIT_CHAR || modeNow == MODE_EDIT_FARM) {
    if (delta != 0) {
      if (delta > 0) {
        for (long i=0;i<delta;i++) { editValue++; if (editValue>MAX_WPM){editValue=MAX_WPM; break;} }
      } else {
        for (long i=0;i<-delta;i++) { editValue--; if (editValue<MIN_WPM){editValue=MIN_WPM; break;} }
      }
      lastEncoderActivity = millis();
      valueMoved = true;
      blinkOn = true; lastBlink = millis();
      updateEditNumber(modeNow, editValue, blinkOn);
      encLastPos = encPos;
    }

    if (millis() - lastBlink >= BLINK_MS) {
      lastBlink = millis();
      blinkOn = !blinkOn;
      updateEditNumber(modeNow, editValue, blinkOn);
    }

    if (millis() - lastEncoderActivity >= EDIT_TIMEOUT) {
      if (modeNow == MODE_EDIT_CHAR) {
        wpm_char = editValue;
        if (wpm_eff > wpm_char) { wpm_eff = wpm_char; saveEffEEP(); }
        saveCharEEP();
        computeFarnsworthTiming(wpm_char, wpm_eff); // sólo aquí
        showLEDTimes();

        modeNow = MODE_EDIT_FARM;
        editValue = wpm_eff;
        encoder.setPosition(0); encLastPos = 0;
        lastEncoderActivity = millis();
        blinkOn = true; lastBlink = millis();
        showEditHeader(MODE_EDIT_FARM);
        updateEditNumber(MODE_EDIT_FARM, editValue, blinkOn);
        valueMoved = false;
      } else {
        if (editValue > wpm_char) editValue = wpm_char;
        wpm_eff = editValue;
        saveEffEEP();
        computeFarnsworthTiming(wpm_char, wpm_eff); // sólo aquí
        showLEDTimes();

        modeNow = MODE_NORMAL;
        showNormal();
      }
    }

  } else {
    encLastPos = encPos;
  }






  // --- KEYER & LEDs
  unsigned long now = millis();
  bool buttonPressed = (digitalRead(PIN_KEYER) == LOW);

 // Detectar flanco de bajada (cuando se pulsa por primera vez)
  if (buttonPressed && !buttonWasPressed) {
    digitalWrite(PIN_LED_CHAR, LOW);
    digitalWrite(PIN_LED_WORD, LOW);
//    Serial.println("Pasa por aquí");
    startTime = millis();
    waitingToTurnOn_C = true;
    waitingToTurnOn_W = true;
    buttonWasPressed = buttonPressed;
  }


// Si estamos esperando y ya pasó el tiempo, enciende el LED
  if ((waitingToTurnOn_C||waitingToTurnOn_W)&& !buttonPressed) {
// Aquí se miran que led hay que encender  
      if (waitingToTurnOn_C && (millis() - startTime >= charGap_ms)) {
          digitalWrite(PIN_LED_CHAR, HIGH);
          waitingToTurnOn_C = false;
          tone(pinBuzzer, 2500, 100);   // 2500 Hz durante 100 ms
          } 
    if (waitingToTurnOn_W && (millis() - startTime >= wordGap_ms)) {
          digitalWrite(PIN_LED_WORD, HIGH);
          waitingToTurnOn_W = false;
          tone(pinBuzzer, 300, 200);   // 300 Hz durante 200 ms
          }
  }
  

  buttonWasPressed = buttonPressed;

// Depuracion solo
/*
Serial.print("startTime: ");
Serial.print(startTime);

Serial.print(".   buttonPressed: ");
Serial.println(buttonPressed);

*/





}





