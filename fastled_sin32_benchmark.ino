/*
    Benchmark for more accurate lookup table and interpolation based approximation for sin and cos
    Copyright (C) 2023 Antti Yliniemi
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifdef CORE_DEBUG_LEVEL
#undef CORE_DEBUG_LEVEL
#endif
#define CORE_DEBUG_LEVEL 3
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "esp_timer.h"

// #define PRINT_ALL_SIN16
#define USE_SIN_32
#include "FastLED.h"

#define TAG "benchmark"

__attribute__((always_inline)) static inline float sqrtInvApprox(float number)          // optimized by Jan Kadlec
{
    union { float f; uint32_t u; } y = {number};
    y.u = 0x5F1FFFF9ul - (y.u >> 1);
    return 0.703952253f * y.f * (2.38924456f - number * y.f * y.f);
}

__attribute__((always_inline)) static inline float sqrtApprox(float number)             // optimized by Jan Kadlec
{
    union { float f; uint32_t u; } y = {number};
    y.u = 0x5F1FFFF9ul - (y.u >> 1);
    return number * 0.703952253f * y.f * (2.38924456f - number * y.f * y.f);
}

void benchmark()
{
    static float testSum = 0;
    static int32_t testSumInt = 0;
    static int64_t timerStart = 0;
    static int64_t timerStop = 0;
    static int32_t timerDelta = 0;
    static float maxError = 0;

    printf("\n");
    printf("sin16(-8 to 8) = %7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d\n", sin16(-8), sin16(-7), sin16(-6), sin16(-5), sin16(-4), sin16(-3), sin16(-2), sin16(-1), sin16(0), sin16(1), sin16(2), sin16(3), sin16(4), sin16(5), sin16(6), sin16(7), sin16(8));
    printf("sin16(16376 - 16392) = %7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d,%7d\n", sin16(16376), sin16(16377), sin16(16378), sin16(16379), sin16(16380), sin16(16381), sin16(16382), sin16(16383), sin16(16384), sin16(16385), sin16(16386), sin16(16387), sin16(16388), sin16(16389), sin16(16390), sin16(16391), sin16(16392));
    #ifdef PRINT_ALL_SIN16
    static bool printedAlready = false;
    printf("sin16(0 - 65535) = \n{");
    if (printedAlready == false)
    {
        for (int i = 0; i < 65536; i++)
        {
            printf("%7d,", sin16(i));
            if (i % 16 == 15) printf("\n");
        }
        printedAlready = true;
    }
    #endif

    vTaskDelay(100);
    #ifdef USE_SIN_32
    printf("USE_SIN_32 is defined");
    #else
    printf("USE_SIN_32 is NOT defined");
    #endif
    printf("\nstarted benchmark\n");

    timerStart = esp_timer_get_time();
    for (int64_t i = 0; i < 10000000; i++)
    {
        testSum += sinf(i * 0.00001);
    }
    timerStop = esp_timer_get_time();
    timerDelta = timerStop - timerStart;
    printf("10M sinf took %d μs\n", timerDelta);
    vTaskDelay(100);
    
    #ifdef USE_SIN_32
    timerStart = esp_timer_get_time();
    for (int32_t i = 0; i < 10000000; i++)
    {
        testSumInt += sin32(i);
    }
    timerStop = esp_timer_get_time();
    timerDelta = timerStop - timerStart;
    printf("10M sin32 took %d μs\n", timerDelta);
    vTaskDelay(100);
    #endif

    #ifdef USE_SIN_32
    timerStart = esp_timer_get_time();
    for (int32_t i = 0; i < 10000000; i++)
    {
        testSumInt += cos32(i);
    }
    timerStop = esp_timer_get_time();
    timerDelta = timerStop - timerStart;
    printf("10M cos32 took %d μs\n", timerDelta);
    vTaskDelay(100);
    #endif

    timerStart = esp_timer_get_time();
    for (int32_t i = 0; i < 10000000; i++)
    {
        testSumInt += sin16(i);
    }
    timerStop = esp_timer_get_time();
    timerDelta = timerStop - timerStart;
    printf("10M sin16 took %d μs\n", timerDelta);
    vTaskDelay(100);
    
    timerStart = esp_timer_get_time();
    for (int32_t i = 0; i < 10000000; i++)
    {
        testSumInt += cos16(i);
    }
    timerStop = esp_timer_get_time();
    timerDelta = timerStop - timerStart;
    printf("10M cos16 took %d μs\n", timerDelta);
    vTaskDelay(100);
    
    #ifdef USE_SIN_32
    timerStart = esp_timer_get_time();
    for (int32_t i = 0; i < 10000000; i++)
    {
        testSumInt += sinArray[(uint8_t)i];
    }
    timerStop = esp_timer_get_time();
    timerDelta = timerStop - timerStart;
    printf("10M sinArray took %d μs\n", timerDelta);
    vTaskDelay(100);
    #endif

    maxError = 0;
    timerStart = esp_timer_get_time();
    for (int32_t i = 0; i < 65536; i++)
    {
        int32_t sin32int = sin16(i);
        double sin16double = sinf((float)i * 2.0 * M_PI / (float)65536.0) * (float)32767.0;
        double possibleMaxError = fabs(sin16double - sin32int);
        if (possibleMaxError > maxError)
        {
            maxError = possibleMaxError;
        }
    }
    timerStop = esp_timer_get_time();
    timerDelta = timerStop - timerStart;
    printf("65K sin - sin16 took %ld μs, maxError = %7f, %7f parts per million\n", timerDelta, maxError, maxError / 32767 * 1000000);
    vTaskDelay(100);
    
    maxError = 0;
    timerStart = esp_timer_get_time();
    for (int32_t i = 0; i < 65536; i++)
    {
        int32_t sin32int = cos16(i);
        double sin16double = cosf((float)i * 2.0 * M_PI / (float)65536.0) * (float)32767.0;
        double possibleMaxError = fabs(sin16double - sin32int);
        if (possibleMaxError > maxError)
        {
            maxError = possibleMaxError;
        }
    }
    timerStop = esp_timer_get_time();
    timerDelta = timerStop - timerStart;
    printf("65K cos - cos16 took %ld μs, maxError = %7f, %7f parts per million\n", timerDelta, maxError, maxError / 32767 * 1000000);
    vTaskDelay(100);
    
    #ifdef USE_SIN_32
    maxError = 0;
    timerStart = esp_timer_get_time();
    for (int32_t i = 0; i < 10000000; i++)
    {
        int32_t sin32int = sin32(i);
        double sin32double = sinf((float)i * 2.0 * M_PI / (float)16777216.0) * (float)32767.0 * 65536.0;
        double possibleMaxError = fabs(sin32double - sin32int);
        if (possibleMaxError > maxError)
        {
            maxError = possibleMaxError;
        }
    }
    timerStop = esp_timer_get_time();
    timerDelta = timerStop - timerStart;
    printf("10M sin - sin32 took %ld μs, maxError = %7f, %7f parts per million\n", timerDelta, maxError, maxError / (32767 * 65536) * 1000000);
    vTaskDelay(100);
    #endif
    
    #ifdef USE_SIN_32
    maxError = 0;
    timerStart = esp_timer_get_time();
    for (int32_t i = 0; i < 10000000; i++)
    {
        int32_t sin32int = cos32(i);
        double sin32double = cosf((float)i * 2.0 * M_PI / (float)16777216.0) * (float)32767.0 * 65536.0;
        double possibleMaxError = fabs(sin32double - sin32int);
        if (possibleMaxError > maxError)
        {
            maxError = possibleMaxError;
        }
    }
    timerStop = esp_timer_get_time();
    timerDelta = timerStop - timerStart;
    printf("10M cos - cos32 took %ld μs, maxError = %7f, %7f parts per million\n", timerDelta, maxError, maxError / (32767 * 65536) * 1000000);
    vTaskDelay(100);
    #endif
    
    timerStart = esp_timer_get_time();
    for (int32_t i = 0; i < 10000000; i++)
    {
        testSum += sqrt(i);
    }
    timerStop = esp_timer_get_time();
    timerDelta = timerStop - timerStart;
    printf("10M sqrt took %ld μs\n", timerDelta);
    vTaskDelay(100);
    
    timerStart = esp_timer_get_time();
    for (int32_t i = 0; i < 10000000; i++)
    {
        testSum += sqrtApprox(i);
    }
    timerStop = esp_timer_get_time();
    timerDelta = timerStop - timerStart;
    printf("10M sqrtApprox took %ld μs\n", timerDelta);
    vTaskDelay(100);
    
    timerStart = esp_timer_get_time();
    for (int32_t i = 0; i < 10000000; i++)
    {
        testSum += sqrtInvApprox(i);
    }
    timerStop = esp_timer_get_time();
    timerDelta = timerStop - timerStart;
    printf("10M sqrtInvApprox took %ld μs\n", timerDelta);
    vTaskDelay(100);
    
    printf("\n%d %f\n", testSumInt, testSum);
}

void setup() {
  // put your setup code here, to run once:
}

void loop() {
  // put your main code here, to run repeatedly:
  benchmark();
}
