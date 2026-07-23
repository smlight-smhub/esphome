/*
 * Copyright 2026 SMLIGHT
 */
#ifdef USE_SG2000

#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/components/sg2000/preferences.h"
#include "esphome/components/sg2000/sg2000_gpio_pin.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"

bool g_simulate_wdt_hang = false;

namespace {
// Helper to read the RISC-V 64-bit time CSR
// The SG2000 C906 core typically has a fixed 25MHz timebase.
inline uint64_t get_riscv_time() {
  uint64_t cycles;
  // C906 is a 64-bit RISC-V core (RV64GC), so we can read the 64-bit time register directly.
  asm volatile("rdtime %0" : "=r"(cycles));
  return cycles;
}
}  // namespace

namespace esphome {

void HOT yield() { taskYIELD(); }

uint32_t IRAM_ATTR HOT millis() { return xTaskGetTickCount() * portTICK_PERIOD_MS; }

uint64_t millis_64() { return static_cast<uint64_t>(xTaskGetTickCount()) * portTICK_PERIOD_MS; }

void HOT delay(uint32_t ms) {
  if (ms == 0) {
    taskYIELD();
  } else {
    // Guarantee AT LEAST 'ms' milliseconds by adding 1 tick, avoiding CPU spinning
    vTaskDelay(pdMS_TO_TICKS(ms) + 1);
  }
}

uint32_t IRAM_ATTR HOT micros() {
  // Assuming 25MHz RISC-V timebase (25 ticks per microsecond)
  return get_riscv_time() / 25;
}

void IRAM_ATTR HOT delayMicroseconds(uint32_t us) {
  uint64_t start = get_riscv_time();
  uint64_t delay_ticks = us * 25;  // 25MHz timebase
  while (get_riscv_time() - start < delay_ticks) {
    // Spin-wait for precise microsecond delay
  }
}

extern "C" void smhub_ipc_request_restart(void);
extern "C" int request_irq(int irqn, int (*handler)(int, void *), unsigned long flags, const char *name, void *priv);

void arch_restart() {
  // Ask the Linux broker to toggle remoteproc state for a clean restart
  smhub_ipc_request_restart();
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

#define WDT2_INTR 39
#define WDT2_BASE 0x03012000
#define DW_WDT_CR (*(volatile uint32_t *) (WDT2_BASE + 0x00))
#define DW_WDT_TORR (*(volatile uint32_t *) (WDT2_BASE + 0x04))
#define DW_WDT_CCVR (*(volatile uint32_t *) (WDT2_BASE + 0x08))
#define DW_WDT_CRR (*(volatile uint32_t *) (WDT2_BASE + 0x0C))
#define DW_WDT_EOI (*(volatile uint32_t *) (WDT2_BASE + 0x14))

#define SYS_CTRL_BASE 0x03000000
#define TOP_WDT_CTRL (*(volatile uint32_t *) (SYS_CTRL_BASE + 0x1A8))

void HOT arch_feed_wdt() {
  if (g_simulate_wdt_hang) {
    return;  // Intentionally starve the WDT for safe testing
  }
  DW_WDT_CRR = 0x76;
}

static int wdt2_isr(int irqn, void *priv) {
  // We do NOT clear the interrupt. We try to ask Linux for a clean restart via IPC.
  // If this hangs (or if Linux fails to restart us), the second WDT timeout will
  // fire and trigger a hard reset of the C906L CPU.
  smhub_ipc_request_restart();
  while (true) {
  }
  return 0;
}

#define TOP_WDT_CTRL_WDT2_RST_SYS_EN (1 << 2)
#define TOP_WDT_CTRL_WDT2_RST_CPU_EN (1 << 6)

void arch_init() {
  sg2000::setup_preferences();

  uint32_t wdt_ctrl = TOP_WDT_CTRL;
  wdt_ctrl &= ~TOP_WDT_CTRL_WDT2_RST_SYS_EN;
  wdt_ctrl |= TOP_WDT_CTRL_WDT2_RST_CPU_EN;
  TOP_WDT_CTRL = wdt_ctrl;

  // Clear any pending WDT interrupt from a previous soft-reset before registering the ISR
  volatile uint32_t eoi = DW_WDT_EOI;
  (void) eoi;

  request_irq(WDT2_INTR, wdt2_isr, 0, "wdt2_isr", nullptr);

  // Set timeout to ~10.7 seconds (0xCC)
  DW_WDT_TORR = 0xCC;

  // Enable WDT in Response Mode = 1 (Interrupt first, then reset)
  DW_WDT_CR = (1 << 1) | (1 << 0);

  arch_feed_wdt();
}

uint32_t arch_get_cpu_cycle_count() {
  uint64_t cycle_count;
  asm volatile("rdcycle %0" : "=r"(cycle_count));
  return static_cast<uint32_t>(cycle_count);
}

uint32_t arch_get_cpu_freq_hz() {
  // SG2000 C906 secondary core typically runs at 700MHz
  return 700000000U;
}

extern "C" uint8_t g_mac_address[6];

void get_mac_address_raw(uint8_t *mac) {
  for (int i = 0; i < 6; i++) {
    mac[i] = g_mac_address[i];
  }
}

bool random_bytes(uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    data[i] = rand() % 256;
  }
  return true;
}

}  // namespace esphome

#include "esphome/components/sg2000/sg2000_gpio_pin.h"

void setup();
void loop();

TaskHandle_t esphome_main_task_handle = nullptr;

#ifdef ESPHOME_FIRMWARE_VERSION_STR
extern "C" {
extern const char ESPHOME_FIRMWARE_VERSION[] __attribute__((used)) = ESPHOME_FIRMWARE_VERSION_STR;
}
#endif

extern "C" void app_setup(void) {
  esphome_main_task_handle = xTaskGetCurrentTaskHandle();
  ::setup();
}

extern "C" void app_loop(void) { ::loop(); }

#endif  // USE_SG2000
