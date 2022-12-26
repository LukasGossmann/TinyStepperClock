/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

#include "WS2812.pio.h"

#include "PWM.h"

const bool IS_RGBW = false;
const uint32_t NUM_PIXELS = 12;
const uint32_t WS2812_PIN = 14;

const PIO pio = pio0;
const int sm = 0;

static inline void put_pixel(uint8_t r, uint8_t g, uint8_t b)
{

    uint32_t value = ((uint32_t)(r) << 16) |
                     ((uint32_t)(g) << 24) |
                     ((uint32_t)(b) << 8);

    pio_sm_put_blocking(pio, sm, value);
}

void pattern_snakes(uint len, uint t)
{
    for (uint i = 0; i < len; ++i)
    {
        uint x = (i + (t >> 1)) % 64;
        if (x < 10)
            put_pixel(0xff, 0, 0);
        else if (x >= 15 && x < 25)
            put_pixel(0, 0xff, 0);
        else if (x >= 30 && x < 40)
            put_pixel(0, 0, 0xff);
        else
            put_pixel(0, 0, 0);
    }
}

void pattern_random(uint len, uint t)
{
    if (t % 8)
        return;
    for (int i = 0; i < len; ++i)
    {
        int value = rand();
        put_pixel(value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF);
    }
}

void pattern_sparkle(uint len, uint t)
{
    if (t % 8)
        return;
    for (int i = 0; i < len; ++i)
    {
        uint8_t sparkle = rand() % 16 ? 0 : 0xff;
        put_pixel(sparkle, sparkle, sparkle);
    }
}

void pattern_color_sparkle(uint len, uint t)
{
    if (t % 8)
        return;
    for (int i = 0; i < len; ++i)
    {
        uint8_t sparkle = rand() % 16 ? 0 : 0xff;
        uint8_t color = rand() % 3;

        switch (color)
        {
        case 0:
            put_pixel(sparkle, 0, 0);
            break;
        case 1:
            put_pixel(0, sparkle, 0);
            break;
        case 2:
            put_pixel(0, 0, sparkle);
            break;
        }
    }
}

void pattern_greys(uint len, uint t)
{
    int max = 100; // let's not draw too much current!
    t %= max;
    for (int i = 0; i < len; ++i)
    {
        uint32_t value = t * 0x10101;
        put_pixel(value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF);
        if (++t >= max)
            t = 0;
    }
}

void pattern_rgbfade(uint len, uint t)
{
    static uint8_t red = 0;
    static uint8_t green = 0;
    static uint8_t blue = 0;

    if (t == 0)
    {
        red = 0;
        green = 0;
        blue = 0;
    }

    uint32_t clampedTime = t % (255 * 4);

    // 0 - 254 = red increasing
    // 255 - 509 = red decreasing, green increasing
    // 510 - 764 = green decreasing, blue increasing
    // 765 - 1019 blue decreasing

    if (clampedTime >= 0 && clampedTime <= 254)
    {
        red++;
    }
    else if (clampedTime >= 255 && clampedTime <= 509)
    {
        red--;
        green++;
    }
    else if (clampedTime >= 510 && clampedTime <= 764)
    {
        green--;
        blue++;
    }
    else if (clampedTime >= 765 && clampedTime <= 1019)
    {
        blue--;
    }

    for (int i = 0; i < len; ++i)
        put_pixel(red, green, blue);
}

typedef void (*pattern)(uint len, uint t);
const struct
{
    uint32_t duration;
    pattern pat;
} pattern_table[] = {
    //{1000, pattern_snakes},
    //{1000, pattern_random},
    {1000, pattern_sparkle},
    {1000, pattern_color_sparkle},
    {1000, pattern_greys},
    {1020, pattern_rgbfade},
};

void ws2812_init()
{
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
}

bool animationActive = false;
uint32_t time = 0;
uint32_t selectedPattern = 0;

void ws2812_animation_tick()
{
    uint32_t duration = pattern_table[selectedPattern].duration;

    if (time >= duration)
    {
        deconfigurePwmFrom50hzTimer(&ws2812_animation_tick);

        for (int i = 0; i < NUM_PIXELS; ++i)
            put_pixel(0, 0, 0);

        animationActive = false;

        return;
    }

    pattern_table[selectedPattern].pat(NUM_PIXELS, time);

    time++;

    clear50hzTimerIrq();
}

void ws2812_do_pattern()
{
    if (animationActive)
        return;

    animationActive = true;
    time = 0;
    selectedPattern = rand() % count_of(pattern_table);

    configurePwmAs50hzTimer(&ws2812_animation_tick);
}
