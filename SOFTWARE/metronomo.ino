#include <LiquidCrystal.h>
#include <RotaryEncoder.h>
#include <EEPROM.h>

// --- Definición de pines ---
#define KEYER_INPUT     2
#define ENCODER_PIN_A   3
#define ENCODER_PIN_B   4
#define LCD_D4          5
#define LCD_D5          6
#define LCD_RS          7
#define LCD_E           8
#define LED_CHAR        9
#define LED_WORD        10
#define LCD_D6          11
#define LCD_D7          12
#define SAVE_BUTTON     21  // Pulsador de memoria activo LOW

// --- Objetos globales ---
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
RotaryEncoder encoder(ENCODER_PIN_A, ENCODER_PIN_B);

unsigned long lastKeyerTime = 0;
int farnsworthWPM = 15;     // Valor por defecto
bool keyerState = false;
bool ledCharOn = false;
bool ledWordOn = false;
bool saveButtonPressed = false;

// --- EEPROM ---
#define EEPROM_ADDR 0  // Dirección donde se guarda el valor WPM

void setup() {
  pinMode(KEYER_INPUT, INPUT_PULLUP);
  pinMode(SAVE_BUTTON, INPUT_PULLUP);
  pinMode(LED_CHAR, OUTPUT);
  pinMode(LED_WORD, OUTPUT);

  digitalWrite(LED_CHAR, LOW);
  digitalWrite(LED_WORD, LOW);

  // Cargar valor WPM desde EEPROM
  int stored = EEPROM.read(EEPROM_ADDR);
  if (stored >= 5 && stored <= 40) farnsworthWPM = stored;
  else farnsworthWPM = 15;  // Valor por defecto si no hay guardado válido

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("CW Visual Metronome");
  delay(1200);
  lcd.clear();
  showWPM();
}

void loop() {
  encoder.tick();

  static int lastPos = 0;
  int newPos = encoder.getPosition();

  if (newPos != lastPos) {
    lastPos = newPos;
    farnsworthWPM = constrain(15 + newPos, 5, 40);
    showWPM();
  }

  // Pulsador de memoria (activo LOW)
  if (digitalRead(SAVE_BUTTON) == LOW && !saveButtonPressed) {
    saveButtonPressed = true;
    EEPROM.update(EEPROM_ADDR, farnsworthWPM);
    lcd.setCursor(0, 1);
    lcd.print("Saved in EEPROM ");
  }
  if (digitalRead(SAVE_BUTTON) == HIGH) saveButtonPressed = false;

  // Detección del keyer (activo LOW)
  bool currentKeyer = (digitalRead(KEYER_INPUT) == LOW);

  if (currentKeyer) {
    lastKeyerTime = millis();
    ledCharOn = ledWordOn = false;
    digitalWrite(LED_CHAR, LOW);
    digitalWrite(LED_WORD, LOW);
  } else {
    unsigned long elapsed = millis() - lastKeyerTime;
    float dotTime = 1200.0 / farnsworthWPM;
    if (elapsed > 3 * dotTime && !ledCharOn) {
      digitalWrite(LED_CHAR, HIGH);
      ledCharOn = true;
    }
    if (elapsed > 7 * dotTime && !ledWordOn) {
      digitalWrite(LED_WORD, HIGH);
      ledWordOn = true;
    }
  }
}

// --- Mostrar WPM en LCD ---
void showWPM() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Farnsworth:");
  lcd.setCursor(0, 1);
  lcd.print(farnsworthWPM);
  lcd.print(" WPM");
}
