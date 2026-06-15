#pragma once

#include "esphome.h"
#include "esphome/components/light/addressable_light.h"
#include <vector>

#include "esphome/components/sg2000/sg2000_arbitration.h"

extern "C" {
void hal_pinmux_config(int io_type);
void hal_clock_enable_spi(uint8_t spi_id);
void hal_reset_deassert_spi(uint8_t spi_id);
}

namespace esphome {
namespace sg2000_ws2812 {

class Sg2000Ws2812 : public light::AddressableLight {
 protected:
  uintptr_t spi_base_;
  uint32_t num_leds_;
  mutable std::vector<uint8_t> buffer_;
  mutable std::vector<uint8_t> effect_data_;

 public:
  Sg2000Ws2812(uintptr_t spi_base, uint32_t num_leds) 
    : spi_base_(spi_base), num_leds_(num_leds) {}

  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::RGB});
    return traits;
  }

  int32_t size() const override { return num_leds_; }
  
  void clear_effect_data() override {
    for (uint32_t i = 0; i < num_leds_; i++) effect_data_[i] = 0;
  }
  
  void setup() override {
    ESP_LOGI("sg2000_ws2812", "Setting up WS2812 strip with %u LEDs on SPI0 (0x%08X)...", num_leds_, spi_base_);

    buffer_.resize(num_leds_ * 3, 0);
    effect_data_.resize(num_leds_, 0);

    if (spi_base_ != 0x04180000) {
      ESP_LOGE("sg2000_ws2812", "Unsupported SPI base! Only SPI0 (0x04180000) is supported.");
      this->mark_failed();
      return;
    }

    if (!sg2000::smhub_hardware_is_released(sg2000::SMHUB_HW_SPI0)) {
      ESP_LOGE("sg2000_ws2812", "Hardware Arbitration FAILED! SPI0 is not authorized by U-Boot.");
      this->mark_failed();
      return;
    }

    // Enable SPI hardware clocks and de-assert reset via HAL
    hal_clock_enable_spi(0);
    hal_reset_deassert_spi(0);

    // Configure PINMUX for SPI0
    hal_pinmux_config(11); // PINMUX_SPI0 (PAD_MIPI_TXM1)

    // Initialize DesignWare SPI for CV181x/SG2000
    volatile uint32_t* spi = (volatile uint32_t*)spi_base_;
    spi[0x08 / 4] = 0; // SSIENR = 0 (Disable SPI)
    spi[0x00 / 4] = 0x0007; // CTRLR0: DFS=8bit, standard SPI, mode 0
    spi[0x14 / 4] = 16; // BAUDR: divider. Adjust for ~6.4 MHz.
    spi[0x10 / 4] = 1; // SER: Slave Enable Register (Slave 0)
    spi[0x08 / 4] = 1; // SSIENR = 1 (Enable SPI)

    for (uint32_t i = 0; i < num_leds_ * 3; i++) {
        for (int bit = 7; bit >= 0; bit--) {
            while ((spi[0x28 / 4] & 0x02) == 0) {
                __asm__ volatile("nop");
            }
            spi[0x60 / 4] = 0xC0;
        }
    }
    for (int i = 0; i < 40; i++) {
        while ((spi[0x28 / 4] & 0x02) == 0) {
            __asm__ volatile("nop");
        }
        spi[0x60 / 4] = 0x00;
    }

    ESP_LOGI("sg2000_ws2812", "WS2812 SPI0 hardware setup complete!");
  }

  void write_state(light::LightState *state) override {
    if (this->is_failed()) return;

    static bool first_write = true;

    volatile uint32_t* spi = (volatile uint32_t*)spi_base_;

    // Write out buffer using SPI bit expansion for WS2812 timing
    for (uint32_t i = 0; i < buffer_.size(); i++) {
        uint8_t byte_val = buffer_[i];
        for (int bit = 7; bit >= 0; bit--) {
            uint8_t spi_byte = (byte_val & (1 << bit)) ? 0xF8 : 0xC0;
            // Wait for TX FIFO not full (SR bit 1)
            while ((spi[0x28 / 4] & 0x02) == 0) {
               __asm__ volatile("nop");
            }
            spi[0x60 / 4] = spi_byte;
        }
    }

    // Send reset pulse (at least 50us of zeros)
    // 50us at 1.25us/byte = 40 bytes
    for (int i = 0; i < 40; i++) {
        while ((spi[0x28 / 4] & 0x02) == 0) {
             __asm__ volatile("nop");
        }
        spi[0x60 / 4] = 0x00;
    }

    if (first_write) {
        ESP_LOGI("sg2000_ws2812", "WS2812 led strip is active! Hardware is operational.");
        first_write = false;
    }

    this->mark_shown_();
  }

 protected:
  light::ESPColorView get_view_internal(int32_t index) const override {
    // WS2812 is GRB
    return {&buffer_[index * 3 + 1],
            &buffer_[index * 3 + 0],
            &buffer_[index * 3 + 2],
            nullptr,
            &effect_data_[index],
            &this->correction_};
  }
};

} // namespace sg2000_ws2812
} // namespace esphome
