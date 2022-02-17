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
    pa_self.i = n;
    while (pa_self.i > 0) {
        pa_pause;
        pa_self.i -= 1;
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
                   pa_co_res(2); pa_use(Delay); pa_use(DetectReleasePress)), 
             Press& press) 
{
    while (true) {
        press = Press::NO;

        pa_await (M5.Btn.wasPressed());

        pa_self.wasReleased = false;
        pa_self.wasPressed = false;
        
        pa_co(2) {
            pa_with_weak (Delay, 3);
            pa_with_weak (DetectReleasePress, pa_self.wasReleased, pa_self.wasPressed);
        } pa_co_end;

        if (pa_self.wasPressed) {
            press = Press::DOUBLE;
        } 
        else if (pa_self.wasReleased) {
            press = Press::SHORT;
        }
        else {
            press = Press::LONG;
        }

        pa_pause;
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
    pa_pause;
} pa_end;

pa_activity (WaitForButtonPressed, pa_ctx(pa_use(Delay)), bool buttonPressed) {
    pa_run (Delay, 4);
    if (!buttonPressed) {
        pa_await (buttonPressed);
    }
} pa_end;

pa_activity (Controller,
             pa_ctx(pa_co_res(2); pa_use(RedToGreenCar); pa_use(RedToGreenPed); 
                    pa_use(GreenToRedCar); pa_use(GreenToRedPed);
                    pa_use(Delay); pa_use(WaitForButtonPressed)),
             Lights& lights, bool& buttonPressed)
{
    // Begin with both lights on red.
    lights.K1.color = Color::RED;
    lights.K2.color = Color::RED;
    
    // Alternate.
    while (true) {
        pa_co(2) {
            pa_with (RedToGreenCar, lights.K2);
            pa_with (GreenToRedPed, lights.K1);
        } pa_co_end;
        
        pa_co(2) {
            pa_with_weak (Delay, 10);
            pa_with_weak (WaitForButtonPressed, buttonPressed);
        } pa_co_end;
        
        pa_co(2) {
            pa_with (GreenToRedCar, lights.K2);
            pa_with (RedToGreenPed, lights.K1);
        } pa_co_end;

        buttonPressed = false;

        pa_run (Delay, 10);
    }
} pa_end;

pa_activity (Presenter, pa_ctx(), const Lights& lights) {
    pa_always {
        setLight<0, true>(lights.K1);
        setLight<3, true>(lights.K2);

        FastLED.show();

    } pa_always_end;
} pa_end;

pa_activity (DoPedPresenter, pa_ctx()) {
    while (true) {
        mainLED = CRGB::Red;
        pa_pause;

        mainLED = CRGB::Black;
        pa_pause;
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
             pa_ctx(pa_co_res(3); Lights lights; 
                    pa_use(Controller); pa_use(Presenter); pa_use(PedPresenter)),
             bool& buttonPressed)
{
    pa_co(3) {
        pa_with (Controller, pa_self.lights, buttonPressed);
        pa_with_weak (Presenter, pa_self.lights);
        pa_with_weak (PedPresenter, buttonPressed);
    } pa_co_end;
} pa_end;

pa_activity (DetectRequest, pa_ctx(), bool btnPressed, bool& pressRequest) {
    pa_always {
        pressRequest |= btnPressed;
    } pa_always_end;
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
             pa_ctx(pa_co_res(2); bool pressRequest;
                    pa_use(RunDayLightProg); pa_use(DetectRequest)), 
             bool btnPressed) {
    pa_co(2) {
        pa_with (DetectRequest, btnPressed, pa_self.pressRequest);
        pa_with (RunDayLightProg, pa_self.pressRequest);
    } pa_co_end;
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
    pa_always {
        value = sample();
    } pa_always_end;
} pa_end;

pa_activity (PotFilter, pa_ctx(uint16_t preValues[5]), uint16_t value, uint16_t& filteredValue) {
    for (auto& preValue : pa_self.preValues) {
        preValue = value;
    }
    filteredValue = value;

    while (true) {
        pa_pause;

        auto sumValue = value;
        auto minValue = value;
        auto maxValue = value;
        for (auto preValue : pa_self.preValues) {
            sumValue += preValue;
            minValue = min(minValue, preValue);
            maxValue = max(maxValue, preValue);
        }
        sumValue -= minValue + maxValue;
        filteredValue = sumValue >> 2;

        for (int i = 1; i < 5; ++i) {
            pa_self.preValues[i - 1] = pa_self.preValues[i];
        }
        pa_self.preValues[4] = value;
    }
} pa_end;

pa_activity (PotProvider, pa_ctx(pa_co_res(2); pa_use(PotSensor); pa_use(PotFilter); uint16_t sensorValue), uint16_t& potValue) {
    pa_co(2) {
        pa_with (PotSensor, pa_self.sensorValue);
        pa_with (PotFilter, pa_self.sensorValue, potValue);
    } pa_co_end;
} pa_end;

pa_activity (PotDetector, pa_ctx(int count; uint16_t prevValue), uint16_t value, bool& didDetect) {
    pa_self.prevValue = value;
    do {
        if (abs(value - pa_self.prevValue) != 0) {
            pa_self.count += 1;
        } else {
            pa_self.count = 0;
        }
        pa_self.prevValue = value;

        pa_pause;
    } while (pa_self.count < 5);

    didDetect = true;
} pa_end;

pa_activity (Dimmer, pa_ctx(), uint16_t value) {
    pa_always {
        // value = 2^12 ~4000 
        // bright = 2^5 ~20
        uint8_t brightness = min(20, value / 200);
        FastLED.setBrightness(20 - brightness);
        FastLED.show();
    } pa_always_end;
} pa_end;

pa_activity (DimmerStopDetector, 
             pa_ctx(bool active; pa_co_res(2);
                    pa_use(Delay); pa_use(PotDetector)), 
             uint16_t value) {
    do {
        pa_self.active = false;
        pa_co(2) {
            pa_with_weak (Delay, 30);
            pa_with_weak (PotDetector, value, pa_self.active);
        } pa_co_end;
    } while (pa_self.active);
} pa_end;

pa_activity (OffLight, pa_ctx(pa_co_res(3); uint16_t potValue; bool active;
                              pa_use(PotProvider); pa_use(PotDetector); 
                              pa_use(Dimmer); pa_use(DimmerStopDetector))) {
    while (true) {
        for (auto& led : leds) {
            led = CRGB::Black;
        }
        FastLED.show();
                    
        pa_co(2) {
            pa_with_weak (PotProvider, pa_self.potValue);
            pa_with (PotDetector, pa_self.potValue, pa_self.active);
        } pa_co_end;

        setAllLights<0, true>();
        setAllLights<3, true>();
        FastLED.show();

        pa_co(3) {
            pa_with_weak (PotProvider, pa_self.potValue);
            pa_with_weak (Dimmer, pa_self.potValue);
            pa_with (DimmerStopDetector, pa_self.potValue);
        } pa_co_end;
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

pa_activity (Driver, pa_ctx(pa_co_res(2); Press press;
                            pa_use(PressRecognizer); pa_use(Dispatcher))) {
    pa_co(2) {
        pa_with (PressRecognizer, pa_self.press);
        pa_with (Dispatcher, pa_self.press);
    } pa_co_end;
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
