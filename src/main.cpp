#include <Arduino.h>
#include <Adafruit_ST7735.h>
#include <RotaryEncoder.h>
#include <EEPROM.h>

Adafruit_ST7735 lcd(7, 8, 9);

#define BUTTON_PIN 2
#define RELAY_PIN 3
#define ROTARY_ENCODER_DATA A2
#define ROTARY_ENCODER_CLK A3

void printTime(unsigned long time);
void updateRotaryEncoder();
void saveTimer();
void updateButton();
void startTimer();
void updateTimer();

RotaryEncoder *encoder = nullptr;

const unsigned long TIMER_INCREMENT_MILLIS = 100;
const unsigned long TIMER_MIN = TIMER_INCREMENT_MILLIS;
const unsigned long TIMER_MAX = 100000 - TIMER_INCREMENT_MILLIS;

const int EEPROM_START = 0x00;

const int DEFAULT_TIMER_DURATION = 10000;
unsigned long timerStart;
unsigned long timerEnd;

unsigned long lastPrintedTime = 0;
unsigned long lastSavedTime = 0;

bool countingDown = false;
bool buttonState;
bool lastButtonState = HIGH;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

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
    lcd.initR(INITR_MINI160x80_PLUGIN);
    lcd.setRotation(1);

    lcd.fillScreen(ST7735_BLACK);
    lcd.setCursor(0, 0);
    lcd.setTextWrap(false);
    lcd.setTextColor(ST7735_WHITE, ST7735_BLACK);
    lcd.setTextSize(5);

    EEPROM.get(EEPROM_START, timerDurationMillis);

    if (timerDurationMillis > TIMER_MAX || timerDurationMillis < TIMER_MIN)
    {
        timerDurationMillis = DEFAULT_TIMER_DURATION;
        EEPROM.put(EEPROM_START, timerDurationMillis);
    }

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);
    encoder = new RotaryEncoder(ROTARY_ENCODER_DATA, ROTARY_ENCODER_CLK, RotaryEncoder::LatchMode::TWO03);

    attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_DATA), checkPosition, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_CLK), checkPosition, CHANGE);
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
    saveTimer();
}

void printTime(unsigned long time)
{
    if (time != lastPrintedTime)
    {
        char buf[20];

        unsigned long seconds = time / 1000;
        unsigned long decimal = (time % 1000) / 10;

        sprintf(buf, "%02lu.%02lu", seconds, decimal);
        lcd.setCursor(0, 0);
        lcd.print(buf);

        const int16_t progressBarStartX = 0;
        const int16_t progressBarHeight = 10;
        const int16_t progressBarStartY = lcd.height() - progressBarHeight;

        if (countingDown)
        {
            lcd.fillRect(progressBarStartX, progressBarStartY, (int16_t)((1 - (timerDurationMillis - time) / (float)timerDurationMillis) * lcd.width()), progressBarHeight, ST7735_ORANGE);
        }
        else
        {
            lcd.fillRect(progressBarStartX, progressBarStartY, lcd.width(), progressBarHeight, ST7735_BLACK);
        }

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
                    delay(1000); // TODO delay bad, replace with something better
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

void saveTimer()
{
    if (countingDown)
    {
        return;
    }

    if (millis() - lastTimeSinceTimerUpdate > AUTOSAVE_INTERVAL_MILLIS)
    {
        EEPROM.put(EEPROM_START, timerDurationMillis);
        lastTimeSinceTimerUpdate = millis();
    }
}
