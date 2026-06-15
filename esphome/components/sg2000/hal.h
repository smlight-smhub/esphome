/*
 * Copyright 2026 SMLIGHT
 */
#pragma once

#ifdef USE_SG2000

#include <cstdint>

#define IRAM_ATTR
#define ISR_INTERNAL
#define PROGMEM
#define HOT __attribute__((hot))

namespace esphome {

__attribute__((always_inline)) inline bool in_isr_context() { return false; }

void yield();
void delay(uint32_t ms);
uint32_t micros();
uint32_t millis();
uint64_t millis_64();
void delayMicroseconds(uint32_t us);  // NOLINT(readability-identifier-naming)
uint32_t arch_get_cpu_cycle_count();
uint32_t arch_get_cpu_freq_hz();
void arch_init();
void arch_feed_wdt();

}  // namespace esphome

#endif  // USE_SG2000
