/*
 * Copyright 2026 SMLIGHT
 */
#if defined(USE_SG2000)
#include "logger.h"
extern "C" uint8_t uart_putc(uint8_t ch);
extern "C" void uart_puts(const char* str);
extern "C" void flush_dcache_range(unsigned long start, unsigned long size);

namespace esphome::logger {

void HOT Logger::write_msg_(const char *msg, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    uart_putc((uint8_t)msg[i]);
  }
  flush_dcache_range(0x8ffe0000, 4096);
}

void Logger::pre_setup() { 
  global_logger = this; 
}

const LogString *Logger::get_uart_selection_() {
  return LOG_STR("DEFAULT (SG2000)");
}

}  // namespace esphome::logger

#endif
