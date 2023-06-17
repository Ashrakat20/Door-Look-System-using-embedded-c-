#include <Key.h>
#include <Keypad.h>
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <LiquidCrystal_I2C.h>



#define IN1_PIN 0
#define IN2_PIN 1
#define GREEN_LED_PIN 2
#define RED_LED_PIN 3
#define BUTTON_PIN 4
#define BUZZER_PIN 5

#define ROWS 4
#define COLS 4

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

uint8_t rowPins[ROWS] = {6, 7, 8, 9};
uint8_t colPins[COLS] = {10, 11, 12, 13};

uint8_t passwordIndex = 0;
char password[5] = "1234";
char enteredPassword[5];

uint8_t buttonState = 1;
uint8_t lastButtonState = 1;
unsigned long lastDebounceTime = 0;
uint32_t debounceDelay = 50;

uint8_t doorLocked = 1;
uint8_t buttonPressed = 0;

unsigned long lockDelay = 2000;

LiquidCrystal_I2C lcd(0x27, 16, 2);


// Function Prototypes
void initializePins();
void initializeLCD();
char getKeyPressed();
void handleKeypadInput(char key);
uint8_t checkPassword();
void unlockDoor();
void lockDoor();
void incorrectPassword();
void clearEnteredPassword();
void buzzBuzzer(uint16_t frequency, uint32_t duration);
void lcdSetCursor(uint8_t col, uint8_t row);
void lcdPrint(const char* text);
void lcdClear();
void buttonInterrupt();
void timerInterrupt();

// Button Interrupt Service Routine
ISR(PCINT1_vect) {
  buttonInterrupt();
}

// Timer Interrupt Service Routine
ISR(TIMER1_COMPA_vect) {
  timerInterrupt();
}

void initializePins() {
  DDRD |= (1 << IN1_PIN) | (1 << IN2_PIN) | (1 << GREEN_LED_PIN) | (1 << RED_LED_PIN);
  DDRD |= (1 << BUZZER_PIN);

  DDRD &= ~(1 << BUTTON_PIN);
  PORTD |= (1 << BUTTON_PIN);

  for (uint8_t i = 0; i < ROWS; i++) {
    DDRB &= ~(1 << rowPins[i]);
    PORTB |= (1 << rowPins[i]);
  }

  for (uint8_t i = 0; i < COLS; i++) {
    DDRB |= (1 << colPins[i]);
    PORTB &= ~(1 << colPins[i]);
  }
}

void initializeLCD() {
  lcd.init();
  lcd.backlight();
}

char getKeyPressed() {
  for (uint8_t r = 0; r < ROWS; r++) {
    PORTB &= ~(1 << rowPins[r]);
    for (uint8_t c = 0; c < COLS; c++) {
      if (!(PINB & (1 << colPins[c]))) {
        _delay_ms(20);
        if (!(PINB & (1 << colPins[c]))) {
          PORTB |= (1 << rowPins[r]);
          return keys[r][c];
        }
      }
    }
    PORTB |= (1 << rowPins[r]);
  }
  return '\0';
}

void handleKeypadInput(char key) {
  if (key == '#') {
    enteredPassword[passwordIndex] = '\0';
    passwordIndex = 0;

    if (checkPassword()) {
      if (doorLocked) {
        unlockDoor();
      } else {
        if (!buttonState) {
          lockDoor();
        }
      }
    } else {
      incorrectPassword();
    }
  } else if (key == '*') {
    clearEnteredPassword();
  } else {
    if (passwordIndex < 4) {
      enteredPassword[passwordIndex] = key;
      passwordIndex++;
    }
  }
}

uint8_t checkPassword() {
  return strcmp(enteredPassword, password) == 0;
}

void unlockDoor() {
  PORTC |= (1 << IN1_PIN);
  PORTC &= ~(1 << IN2_PIN);
  PORTD |= (1 << GREEN_LED_PIN);
  PORTD &= ~(1 << RED_LED_PIN);
  lcdClear();
  lcdSetCursor(0, 0);
  lcdPrint("Access Given");
  _delay_ms(2000);
  PORTC &= ~((1 << IN1_PIN) | (1 << IN2_PIN));
  PORTD &= ~(1 << GREEN_LED_PIN);
  lcdClear();
  lcdSetCursor(0, 0);
  lcdPrint("Door Unlocked");
  _delay_ms(2000);
  lcdClear();
  lcdSetCursor(0, 0);
  lcdPrint("Enter Password:");
  clearEnteredPassword();
  doorLocked = 0;
}

void lockDoor() {
  PORTC &= ~((1 << IN1_PIN) | (1 << IN2_PIN));
  PORTC |= (1 << IN2_PIN);
  PORTD &= ~(1 << GREEN_LED_PIN);
  PORTD |= (1 << RED_LED_PIN);
  lcdClear();
  lcdSetCursor(0, 0);
  lcdPrint("Door Locked");
  _delay_ms(2000);
  PORTC &= ~((1 << IN1_PIN) | (1 << IN2_PIN));
  PORTD &= ~(1 << RED_LED_PIN);
  lcdClear();
  lcdSetCursor(0, 0);
  lcdPrint("Enter Password:");
  clearEnteredPassword();
  doorLocked = 1;
}

void incorrectPassword() {
  PORTD &= ~(1 << GREEN_LED_PIN);
  PORTD |= (1 << RED_LED_PIN);
  buzzBuzzer(1000, 500);
  lcdClear();
  lcdSetCursor(0, 0);
  lcdPrint("Incorrect Password");
  _delay_ms(2000);
  lcdClear();
  lcdSetCursor(0, 0);
  lcdPrint("Enter Password:");
  clearEnteredPassword();
  PORTD &= ~(1 << RED_LED_PIN);
}

void clearEnteredPassword() {
  memset(enteredPassword, 0, sizeof(enteredPassword));
  lcdSetCursor(0, 1);
  lcdPrint("    ");
}

void buzzBuzzer(uint16_t frequency, uint32_t duration) {
  uint32_t period = 1000000 / frequency;
  uint32_t pulseDuration = (period * duration) / 1000;

  for (uint32_t i = 0; i < pulseDuration; i++) {
    PORTD |= (1 << BUZZER_PIN);
    _delay_us(period / 2);
    PORTD &= ~(1 << BUZZER_PIN);
    _delay_us(period / 2);
  }
}

void lcdSetCursor(uint8_t col, uint8_t row) {
  lcd.setCursor(col, row);
}

void lcdPrint(const char* text) {
  lcd.print(text);
}

void lcdClear() {
  lcd.clear();
}

void buttonInterrupt() {
  buttonState = bit_is_clear(PIND, BUTTON_PIN);

  if (buttonState != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (buttonState != buttonPressed) {
      buttonPressed = buttonState;

      if (doorLocked) {
        unlockDoor();
      } else {
        lockDoor();
      }
    }
  }

  lastButtonState = buttonState;
}


void timerInterrupt() {
  unsigned long currentTime = millis();
  if ((currentTime - lastDebounceTime) >= lockDelay && !doorLocked) {
    lockDoor();
  }
}

int main() {
  initializePins();
  initializeLCD();

  lcdPrint("Enter Password:");

  // Configure Button Interrupt
  PCICR |= (1 << PCIE1);
  PCMSK1 |= (1 << PCINT12);

  // Configure Timer Interrupt
  TCCR1B |= (1 << WGM12);
  OCR1A = 15624;  // Timer Count for 1 second with 16MHz clock
  TIMSK1 |= (1 << OCIE1A);
  TCCR1B |= ((1 << CS12) | (1 << CS10));  // Start Timer with 1024 prescaler

  // Enable Global Interrupts
  sei();

  while (1) {
    char key = getKeyPressed();

    if (key != '\0') {
      handleKeypadInput(key);
      lastDebounceTime = millis();
    }
  }

  return 0;
}