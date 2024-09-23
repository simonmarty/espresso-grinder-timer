#include <Arduino.h>
#include <LiquidCrystal.h>
#include <RotaryEncoder.h>
#include <EEPROM.h>

LiquidCrystal lcd(12, 11, 7, 6, 5, 4);

void printTime(unsigned long time);

RotaryEncoder *encoder = nullptr;

const int EEPROM_START = 0;
const int BUTTON_PIN = 8;

void checkPosition()
{
  encoder->tick(); // just call tick() to check the state.
}

unsigned long timerDurationMillis = 0;
void setup()
{
  // put your setup code here, to run once:
  lcd.begin(16, 2);

  EEPROM.get(EEPROM_START, timerDurationMillis);

  printTime(timerDurationMillis);

  if (timerDurationMillis != 10000)
  {
    timerDurationMillis = 10000;
    EEPROM.put(EEPROM_START, timerDurationMillis);
  }

  encoder = new RotaryEncoder(A2, A3, RotaryEncoder::LatchMode::TWO03);

  attachInterrupt(digitalPinToInterrupt(A2), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(A3), checkPosition, CHANGE);
}

void loop()
{
  static int pos = 0;

  encoder->tick();
  int newPos = encoder->getPosition();

  if (pos != newPos)
  {
    switch (encoder->getDirection())
    {
    case RotaryEncoder::Direction::CLOCKWISE:
      timerDurationMillis += 100;
      break;
    case RotaryEncoder::Direction::COUNTERCLOCKWISE:
      timerDurationMillis -= 100;
      break;
    case RotaryEncoder::Direction::NOROTATION:
      break;
    }

    printTime(timerDurationMillis);
    pos = newPos;
  }
}

void printTime(unsigned long time)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(time / 1000);
  lcd.print(".");
  unsigned long decimal_part = time % 1000 / 10; // remove the 1000ths place
  lcd.print(decimal_part / 10);
  lcd.print(decimal_part % 10);
}
