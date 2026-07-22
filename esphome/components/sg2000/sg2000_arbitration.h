/*
 * Copyright 2026 SMLIGHT
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#include <hal_pinmux.h>
}

namespace esphome {
namespace sg2000 {
#endif

// Placed exactly 8 bytes BELOW the UART crash buffer.
// (Adjust UART_BUFFER_BASE_ADDR to match your actual linker/UART buffer start address)
#define UART_BUFFER_BASE_ADDR 0x8ffe0000

#define SMHUB_ARBITRATION_ADDR_MAGIC (UART_BUFFER_BASE_ADDR - 8)
#define SMHUB_ARBITRATION_ADDR_MASK (UART_BUFFER_BASE_ADDR - 4)

#define SMHUB_ARBITRATION_MAGIC_WORD 0x534D4857  // "SMHW"

// X-Macro List for Peripherals
#define SMHUB_PERIPHERAL_LIST(X) \
  X(SPI0, 0) \
  X(SPI1, 1) \
  X(SPI3, 2) \
  X(I2C2, 3) \
  X(I2C4, 4) \
  X(UART2, 5) \
  X(UART3, 6) \
  X(PWM0, 7) \
  X(PWM1, 8) \
  X(PWM2, 9) \
  X(PWM3, 10) \
  X(ADC1, 11) \
  X(ADC2, 12)

#define SMHUB_DECLARE_ENUM(name, shift) enum { SMHUB_HW_##name = (1 << (shift)) };
SMHUB_PERIPHERAL_LIST(SMHUB_DECLARE_ENUM)

static inline bool smhub_hardware_is_released(uint32_t peripheral_bit) {
  volatile uint32_t *magic = (volatile uint32_t *) SMHUB_ARBITRATION_ADDR_MAGIC;
  volatile uint32_t *mask = (volatile uint32_t *) SMHUB_ARBITRATION_ADDR_MASK;

  if (*magic != SMHUB_ARBITRATION_MAGIC_WORD) {
    // U-Boot did not run the overlay script, or magic word is corrupted!
    // Fail safe: Deny access to prevent kernel panics.
    return false;
  }

  return (*mask & peripheral_bit) != 0;
}

#ifdef __cplusplus
inline void pad_config(uint32_t pin_reg_offset, bool pull_up, bool pull_down, uint8_t drive_strength,
                       uint8_t schmitt = 0, bool slew_fast = false, bool bus_hold = false) {
  hal_pad_config(pin_reg_offset, pull_up, pull_down, drive_strength, schmitt, slew_fast, bus_hold);
}

inline void pinmux_config(uint32_t pin_reg_offset, uint32_t func_val) {
  hal_pinmux_config_raw(pin_reg_offset, func_val);
}

}  // namespace sg2000
}  // namespace esphome
#endif
