#include <Arduino.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 7, 6, 5, 4);
unsigned long timerEnd;

const unsigned long DURATION = 5000;
const int BUTTON_PIN = 8;
const int RELAY_PIN = 2;

unsigned long lastReset = 0;
bool isCounting = false;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
bool previous = HIGH;

void printTime(unsigned long time);

void setup()
{
  // put your setup code here, to run once:
  lcd.begin(16, 2);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
}

void loop()
{
  bool current = digitalRead(8);

  if (current != previous)
  {
    lastDebounceTime = millis();
  }

  if (millis() - lastDebounceTime > debounceDelay)
  {
    if (current != previous)
    {
      previous = current;
    }

    if (current == LOW && !isCounting)
    {
      isCounting = true;
    }

    timerEnd = millis() + DURATION;
    digitalWrite(2, LOW);
    isCounting = true;
    lcd.clear();
    delay(20); // debounce
  }
  previous = current;

  if (isCounting)
  {
    if (millis() >= timerEnd)
    {
      digitalWrite(2, HIGH);
      isCounting = false;
      printTime(DURATION);
    }
    else
    {
      unsigned long timerLeft = timerEnd - millis();
      printTime(timerLeft);
    }
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
