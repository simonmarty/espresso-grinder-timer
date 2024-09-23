#include <Arduino.h>
#include <LiquidCrystal.h>
#include <RotaryEncoder.h>
#include <EEPROM.h>

LiquidCrystal lcd(12, 11, 7, 6, 5, 4);

void printTime(unsigned long time);
void updateRotaryEncoder();
void autoSaveTimer();
void saveTimer();
void updateButton();
void startTimer();
void updateTimer();

RotaryEncoder *encoder = nullptr;

const unsigned long TIMER_INCREMENT_MILLIS = 100;
const unsigned long TIMER_MIN = TIMER_INCREMENT_MILLIS;
const unsigned long TIMER_MAX = 100000 - TIMER_INCREMENT_MILLIS;

const int EEPROM_START = 0;
const int BUTTON_PIN = 8;
const int DEFAULT_TIMER_DURATION = 10000;
unsigned long timerEnd;

const int ROTARY_ENCODER_PIN1 = A2;
const int ROTARY_ENCODER_PIN2 = A3;

unsigned long lastPrintedTime = 0;
unsigned long lastSavedTime = 0;

bool countingDown = false;
bool buttonState;
bool lastButtonState = HIGH;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

const int RELAY_PIN = 2;

unsigned long lastTimeAutoSave;
const unsigned long AUTOSAVE_INTERVAL_MILLIS = 10000;
unsigned long lastTimeSinceTimerUpdate;

void checkPosition()
{
    encoder->tick(); // just call tick() to check the state.
}

unsigned long timerDurationMillis = 0;
void setup()
{
    // put your setup code here, to run once:
    lcd.begin(16, 2);
    Serial.begin(57600);

    EEPROM.get(EEPROM_START, timerDurationMillis);

    if (timerDurationMillis > TIMER_MAX || timerDurationMillis < TIMER_MIN)
    {
        timerDurationMillis = DEFAULT_TIMER_DURATION;
        EEPROM.put(EEPROM_START, timerDurationMillis);
    }

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);
    encoder = new RotaryEncoder(ROTARY_ENCODER_PIN1, ROTARY_ENCODER_PIN2, RotaryEncoder::LatchMode::TWO03);

    attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_PIN1), checkPosition, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_PIN2), checkPosition, CHANGE);
}

void loop()
{
    updateRotaryEncoder();
    updateButton();

    if (countingDown)
    {
        printTime(timerEnd - millis());
    }
    else
    {
        printTime(timerDurationMillis);
    }

    updateTimer();
    autoSaveTimer();
}

void printTime(unsigned long time)
{
    if (time != lastPrintedTime)
    {
        char buf[20];

        unsigned long seconds = time / 1000;
        unsigned long decimal = (time % 1000) / 10;

        sprintf(buf, "%02lu.%02lu", seconds, decimal);
        // lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(buf);
        // lcd.print(time / 1000);
        // lcd.print(".");
        // unsigned long decimal_part = time % 1000 / 10; // remove the 1000ths place
        // lcd.print(decimal_part / 10);
        // lcd.print(decimal_part % 10);

        lastPrintedTime = time;
    }
}

void updateRotaryEncoder()
{
    static int rotaryEncoderPosition = 0;
    encoder->tick();
    int newPos = encoder->getPosition();

    if (rotaryEncoderPosition != newPos)
    {
        switch (encoder->getDirection())
        {
        case RotaryEncoder::Direction::CLOCKWISE:
            if (timerDurationMillis >= TIMER_MAX)
                break;
            timerDurationMillis += TIMER_INCREMENT_MILLIS;
            lastTimeSinceTimerUpdate = millis();
            break;
        case RotaryEncoder::Direction::COUNTERCLOCKWISE:
            if (timerDurationMillis <= TIMER_MIN)
                break;
            timerDurationMillis -= TIMER_INCREMENT_MILLIS;
            lastTimeSinceTimerUpdate = millis();
            break;
        case RotaryEncoder::Direction::NOROTATION:
            break;
        }

        rotaryEncoderPosition = newPos;
    }
}

void updateButton()
{
    int reading = digitalRead(BUTTON_PIN);

    if (reading != lastButtonState)
    {
        lastDebounceTime = millis();
    }

    if (millis() - lastDebounceTime > debounceDelay)
    {
        if (reading != buttonState)
        {
            buttonState = reading;

            if (buttonState == LOW)
            {
                if (countingDown)
                {
                    countingDown = false;
                    digitalWrite(RELAY_PIN, HIGH);
                    timerEnd = millis();
                }
                else
                {
                    startTimer();
                }
            }
        }
    }

    lastButtonState = reading;
}

void startTimer()
{
    countingDown = true;
    digitalWrite(RELAY_PIN, LOW);
    timerEnd = millis() + timerDurationMillis;
}

void updateTimer()
{
    if (countingDown)
    {
        if (millis() >= timerEnd)
        {
            countingDown = false;
            digitalWrite(RELAY_PIN, HIGH);
        }
    }
}

void autoSaveTimer()
{
    if (countingDown)
    {
        return;
    }

    if (millis() - lastTimeSinceTimerUpdate > AUTOSAVE_INTERVAL_MILLIS)
    {
        saveTimer();
        lastTimeSinceTimerUpdate = millis();
    }
}

void saveTimer()
{
    unsigned long oldTimerDurationMillis;
    EEPROM.get(EEPROM_START, oldTimerDurationMillis);

    if (oldTimerDurationMillis != timerDurationMillis)
    {
        Serial.println("Saving timer\n");
        EEPROM.put(EEPROM_START, timerDurationMillis);
    }
}
