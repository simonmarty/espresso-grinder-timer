#include <Arduino.h>
#include <LiquidCrystal.h>
#include <RotaryEncoder.h>
#include <EEPROM.h>

LiquidCrystal lcd(5, 4, 9, 8, 7, 6);

void printTime(unsigned long time);
void updateRotaryEncoder();
void autoSaveTimer();
void saveTimer();
void updateButton();
void startTimer();
void updateTimer();
void updateBacklight();

RotaryEncoder *encoder = nullptr;

const int BACKLIGHT_PIN = 15;

const unsigned long TIMER_INCREMENT_MILLIS = 100;
const unsigned long TIMER_MIN = TIMER_INCREMENT_MILLIS;
const unsigned long TIMER_MAX = 100000 - TIMER_INCREMENT_MILLIS;
const unsigned long BACKLIGHT_DELAY = 5000;

const int EEPROM_START = 0;
const int BUTTON_PIN = 10;

const int DEFAULT_TIMER_DURATION = 10000;
unsigned long timerStart;
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

const int RELAY_PIN = 14;

unsigned long lastTimeAutoSave;
const unsigned long AUTOSAVE_INTERVAL_MILLIS = 10000;
unsigned long lastTimeSinceTimerUpdate;

unsigned long lastInputTime = 0;

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
    pinMode(BACKLIGHT_PIN, OUTPUT);
    digitalWrite(BACKLIGHT_PIN, HIGH);
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
        printTime(millis() - timerStart);
    }
    else
    {
        printTime(timerDurationMillis);
    }

    updateTimer();
    autoSaveTimer();

    updateBacklight();
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

        lastInputTime = millis();
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
                    printTime(timerEnd - timerStart);
                    delay(1000); // remove this
                }
                else
                {
                    startTimer();
                }

                lastInputTime = millis();
            }
        }
    }

    lastButtonState = reading;
}

void startTimer()
{
    countingDown = true;
    digitalWrite(RELAY_PIN, LOW);
    timerStart = millis();
    timerEnd = timerStart + timerDurationMillis;
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

void updateBacklight()
{
    if(lastInputTime + BACKLIGHT_DELAY < millis() && digitalRead(BACKLIGHT_PIN) == LOW)
    {
        digitalWrite(BACKLIGHT_PIN, HIGH);
    }
    else {
        digitalWrite(BACKLIGHT_PIN, LOW);
    }
}