/* ttl_main
 *
 * Copyright (c) 2022, Framework Labs.
 */

#include "proto_activities.h"

#include <M5Atom.h>
#include <FastLED.h>

// Globals

constexpr auto POT_PORT = 32;
constexpr auto LED_COUNT = 6;
static CRGB mainLED;
static CRGB leds[LED_COUNT];

// Timing

pa_activity (Delay, pa_ctx(unsigned i), unsigned n) {
    self->i = n;
    while (self->i > 0) {
        pa_await (true);
        self->i -= 1;
    }
} pa_end;

// Button

enum class Press : uint8_t {
    NO,
    SHORT,
    DOUBLE,
    LONG
};

pa_activity (DetectReleasePress, pa_ctx(), bool& wasReleased, bool& wasPressed) {
    pa_await (M5.Btn.wasReleased());
    wasReleased = true;
    pa_await (M5.Btn.wasPressed());
    wasPressed = true;
} pa_end;

pa_activity (PressRecognizer, 
             pa_ctx(bool wasPressed; bool wasReleased; 
                   pa_codef(2); pa_use(Delay); pa_use(DetectReleasePress)), 
             Press& press) 
{
    while (true) {
        press = Press::NO;

        pa_await (M5.Btn.wasPressed());

        self->wasReleased = false;
        self->wasPressed = false;
        
        pa_cobegin(2) {
            pa_with_weak (Delay, 3);
            pa_with_weak (DetectReleasePress, self->wasReleased, self->wasPressed);
        } pa_coend;

        if (self->wasPressed) {
            press = Press::DOUBLE;
        } 
        else if (self->wasReleased) {
            press = Press::SHORT;
        }
        else {
            press = Press::LONG;
        }

        pa_await (true);
    }
} pa_end;

// Lights

enum class Color {
    RED,
    RED_YELLOW,
    YELLOW,
    GREEN
};

struct Light {
    Color color;
    
    std::string colorName() const {
        switch (color) {
            case Color::RED: return "RED";
            case Color::RED_YELLOW: return "RED-YELLOW";
            case Color::YELLOW: return "YELLOW";
            case Color::GREEN: return "GREEN";
        }
        return {};
    }
};

struct Lights {
    Light K1, K2;
};

template <uint start, bool invert>
static void setLight(Light light) {
    static_assert(start <= NUM_LEDS - 3, "");

    constexpr auto redIndex = invert ? start + 2 : start;
    constexpr auto yellowIndex = start + 1;
    constexpr auto greenIndex = invert ? start : start + 2;

    switch (light.color) {
        case Color::GREEN:
            leds[redIndex] = CRGB::Black;
            leds[yellowIndex] = CRGB::Black;
            leds[greenIndex] = CRGB::Green;
            break;
        case Color::YELLOW:
            leds[redIndex] = CRGB::Black;
            leds[yellowIndex] = CRGB::Yellow;
            leds[greenIndex] = CRGB::Black;
            break;
        case Color::RED_YELLOW:
            leds[redIndex] = CRGB::Red;
            leds[yellowIndex] = CRGB::Yellow;
            leds[greenIndex] = CRGB::Black;
            break;
        case Color::RED:
            leds[redIndex] = CRGB::Red;
            leds[yellowIndex] = CRGB::Black;
            leds[greenIndex] = CRGB::Black;
            break;
    }
}

template <uint start, bool invert>
void setAllLights() {
    static_assert(start <= NUM_LEDS - 3, "");

    constexpr auto redIndex = invert ? start + 2 : start;
    constexpr auto yellowIndex = start + 1;
    constexpr auto greenIndex = invert ? start : start + 2;

    leds[redIndex] = CRGB::Red;
    leds[yellowIndex] = CRGB::Yellow;
    leds[greenIndex] = CRGB::Green;
}

// DayLight

pa_activity (RedToGreenCar, pa_ctx(pa_use(Delay)), Light& light) {
    pa_run (Delay, 1);
    light.color = Color::RED_YELLOW;
    pa_run (Delay, 1);
    light.color = Color::GREEN;
} pa_end;

pa_activity (RedToGreenPed, pa_ctx(pa_use(Delay)), Light& light) {
    pa_run (Delay, 3);
    light.color = Color::GREEN;
} pa_end;

pa_activity (GreenToRedCar, pa_ctx(pa_use(Delay)), Light& light) {
    pa_run (Delay, 1);
    light.color = Color::YELLOW;
    pa_run (Delay, 1);
    light.color = Color::RED;
} pa_end;

pa_activity (GreenToRedPed, pa_ctx(pa_use(Delay)), Light& light) {
    light.color = Color::RED;
    pa_await (true);
} pa_end;

pa_activity (WaitForButtonPressed, pa_ctx(pa_use(Delay)), bool buttonPressed) {
    pa_run (Delay, 4);
    if (!buttonPressed) {
        pa_await (buttonPressed);
    }
} pa_end;

pa_activity (Controller,
             pa_ctx(pa_codef(2); pa_use(RedToGreenCar); pa_use(RedToGreenPed); 
                    pa_use(GreenToRedCar); pa_use(GreenToRedPed);
                    pa_use(Delay); pa_use(WaitForButtonPressed)),
             Lights& lights, bool& buttonPressed)
{
    // Begin with both lights on red.
    lights.K1.color = Color::RED;
    lights.K2.color = Color::RED;
    
    // Alternate.
    while (true) {
        pa_cobegin(2) {
            pa_with (RedToGreenCar, lights.K2);
            pa_with (GreenToRedPed, lights.K1);
        } pa_coend;
        
        pa_cobegin(2) {
            pa_with_weak (Delay, 10);
            pa_with_weak (WaitForButtonPressed, buttonPressed);
        } pa_coend;
        
        pa_cobegin(2) {
            pa_with (GreenToRedCar, lights.K2);
            pa_with (RedToGreenPed, lights.K1);
        } pa_coend;

        buttonPressed = false;

        pa_run (Delay, 10);
    }
} pa_end;

pa_activity (Presenter, pa_ctx(), const Lights& lights) {
    while (true) {
        setLight<0, true>(lights.K1);
        setLight<3, true>(lights.K2);

        FastLED.show();

        pa_await (true);
    }
} pa_end;

pa_activity (DoPedPresenter, pa_ctx()) {
    while (true) {
        mainLED = CRGB::Red;
        pa_await (true);

        mainLED = CRGB::Black;
        pa_await (true);
    }
} pa_end;

void clearPedPresentation() {
    mainLED = CRGB::Black;
    FastLED.show();
}

pa_activity (PedPresenter, pa_ctx(pa_use(DoPedPresenter)), bool buttonPressed) {
    while (true) {
        if (buttonPressed) {
            pa_when_abort (!buttonPressed, DoPedPresenter);
        }
        clearPedPresentation();
        pa_await (buttonPressed);
    }
} pa_end;

pa_activity (DayLightProg,
             pa_ctx(pa_codef(3); Lights lights; 
                    pa_use(Controller); pa_use(Presenter); pa_use(PedPresenter)),
             bool& buttonPressed)
{
    pa_cobegin(3) {
        pa_with (Controller, self->lights, buttonPressed);
        pa_with_weak (Presenter, self->lights);
        pa_with_weak (PedPresenter, buttonPressed);
    } pa_coend;
} pa_end;

pa_activity (DetectRequest, pa_ctx(), bool btnPressed, bool& pressRequest) {
    while (true) {
        pressRequest |= btnPressed;
        pa_await (true);
    }
} pa_end;

pa_use(DayLightProg);

pa_activity (RunDayLightProg, pa_ctx(pa_use(Delay)), bool& pressRequest) {
    memset(&DayLightProg_inst, 0, sizeof(DayLightProg_frame));
    while (true) {
        // We run the day-light mode as a separate program at 1Hz to simplify 
        // and potenitally optimize it a bit.
        pa_tick(DayLightProg, pressRequest);
        pa_run (Delay, 10);
    }
} pa_end;

pa_activity (DayLight, 
             pa_ctx(pa_codef(2); bool pressRequest;
                    pa_use(RunDayLightProg); pa_use(DetectRequest)), 
             bool btnPressed) {
    pa_cobegin(2) {
        pa_with (DetectRequest, btnPressed, self->pressRequest);
        pa_with (RunDayLightProg, self->pressRequest);
    } pa_coend;
} pa_end;

// NightLight

pa_activity (NightLight, pa_ctx(pa_use(Delay))) {
    for (auto& led : leds) {
        led = CRGB::Black;
    }
    FastLED.show();

    while (true) {
        for (auto i = 1; i < NUM_LEDS; i += 3) {
            leds[i] = CRGB::Yellow;
        }
        FastLED.show();
        pa_run (Delay, 10);

        for (auto i = 1; i < NUM_LEDS; i += 3) {
            leds[i] = CRGB::Black;
        }
        FastLED.show();
        pa_run (Delay, 10);
    }
} pa_end;

// OffLight

/// This does multisampling over a period of 50ms as the potentiometer provides
/// very unstable values.
uint16_t sample() {
    auto value = analogRead(POT_PORT);
    auto sumValue = value;
    auto minValue = value;
    auto maxValue = value;

    for (int i = 1; i < 6; ++i) {
        delay(10); // This will sum up to 50 ms.
        value = analogRead(POT_PORT);
        sumValue += value;
        minValue = min(minValue, value);
        maxValue = max(maxValue, value);
    }

    // Get rid of maximum and minimum readings.
    sumValue -= minValue + maxValue;

    return sumValue >> 2;
}

pa_activity (PotSensor, pa_ctx(), uint16_t& value) {
    while (true) {
        value = sample();
        pa_await (true);
    }
} pa_end;

pa_activity (PotFilter, pa_ctx(uint16_t preValues[5]), uint16_t value, uint16_t& filteredValue) {
    for (auto& preValue : self->preValues) {
        preValue = value;
    }
    filteredValue = value;

    while (true) {
        pa_await (true);

        auto sumValue = value;
        auto minValue = value;
        auto maxValue = value;
        for (auto preValue : self->preValues) {
            sumValue += preValue;
            minValue = min(minValue, preValue);
            maxValue = max(maxValue, preValue);
        }
        sumValue -= minValue + maxValue;
        filteredValue = sumValue >> 2;

        for (int i = 1; i < 5; ++i) {
            self->preValues[i - 1] = self->preValues[i];
        }
        self->preValues[4] = value;
    }
} pa_end;

pa_activity (PotProvider, pa_ctx(pa_codef(2); pa_use(PotSensor); pa_use(PotFilter); uint16_t sensorValue), uint16_t& potValue) {
    pa_cobegin(2) {
        pa_with (PotSensor, self->sensorValue);
        pa_with (PotFilter, self->sensorValue, potValue);
    } pa_coend;
} pa_end;

pa_activity (PotDetector, pa_ctx(int count; uint16_t prevValue), uint16_t value, bool& didDetect) {
    self->prevValue = value;
    do {
        if (abs(value - self->prevValue) != 0) {
            self->count += 1;
        } else {
            self->count = 0;
        }
        self->prevValue = value;

        pa_await (true);
    } while (self->count < 5);

    didDetect = true;
} pa_end;

pa_activity (Dimmer, pa_ctx(), uint16_t value) {
    while (true) {
        {
            // value = 2^12 ~4000 
            // bright = 2^5 ~20
            uint8_t brightness = min(20, value / 200);
            FastLED.setBrightness(20 - brightness);
        }
        FastLED.show();
        pa_await (true);
    }
} pa_end;

pa_activity (DimmerStopDetector, 
             pa_ctx(bool active; pa_codef(2);
                    pa_use(Delay); pa_use(PotDetector)), 
             uint16_t value) {
    do {
        self->active = false;
        pa_cobegin(2) {
            pa_with_weak (Delay, 30);
            pa_with_weak (PotDetector, value, self->active);
        } pa_coend;
    } while (self->active);
} pa_end;

pa_activity (OffLight, pa_ctx(pa_codef(3); uint16_t potValue; bool active;
                              pa_use(PotProvider); pa_use(PotDetector); 
                              pa_use(Dimmer); pa_use(DimmerStopDetector))) {
    while (true) {
        for (auto& led : leds) {
            led = CRGB::Black;
        }
        FastLED.show();
                    
        pa_cobegin(2) {
            pa_with_weak (PotProvider, self->potValue);
            pa_with (PotDetector, self->potValue, self->active);
        } pa_coend;

        setAllLights<0, true>();
        setAllLights<3, true>();
        FastLED.show();

        pa_cobegin(3) {
            pa_with_weak (PotProvider, self->potValue);
            pa_with_weak (Dimmer, self->potValue);
            pa_with (DimmerStopDetector, self->potValue);
        } pa_coend;
    }
} pa_end;

// Driver

pa_activity (Dispatcher, pa_ctx(pa_use(DayLight); pa_use(NightLight); pa_use(OffLight)), Press press) {
    while (true) {
        if (press == Press::NO || press == Press::SHORT) {
            pa_when_abort (press == Press::LONG || press == Press::DOUBLE, DayLight, press == Press::SHORT);
            clearPedPresentation();
        }
        if (press == Press::DOUBLE) {
            pa_when_abort (press != Press::NO, NightLight);
        }
        if (press == Press::LONG) {
            pa_when_abort (press != Press::NO, OffLight);
        }
    }
} pa_end;

pa_activity (Driver, pa_ctx(pa_codef(2); Press press;
                            pa_use(PressRecognizer); pa_use(Dispatcher))) {
    pa_cobegin(2) {
        pa_with (PressRecognizer, self->press);
        pa_with (Dispatcher, self->press);
    } pa_coend;
} pa_end;

pa_use(Driver);

// Setup and Loop

void setup() {
    setCpuFrequencyMhz(80);

    M5.begin();

    FastLED.addLeds<NEOPIXEL, 19>(leds, sizeof(leds) / sizeof(CRGB));
    FastLED.addLeds<NEOPIXEL, 27>(&mainLED, 1);
    FastLED.setBrightness(10);
}

void loop() {
    TickType_t prevWakeTime = xTaskGetTickCount();

    while (true) {
        M5.update();

        pa_tick(Driver);

        // We run at 10 Hz.
        vTaskDelayUntil(&prevWakeTime, 100);
    }
}
