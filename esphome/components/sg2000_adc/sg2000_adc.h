/*
 * Copyright (C) 2026 SMLIGHT
 * All rights reserved.
 *
 * This file is part of the SMHUB RTOS project.
 */
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/log.h"

extern "C" {
#include "adc.h"
#include "hal_pinmux.h"
#include "cv181x_reg_fmux_gpio.h"
}
#include "esphome/components/sg2000/sg2000_arbitration.h"

namespace esphome {
namespace sg2000_adc {

static const char *const TAG = "sg2000_adc";

class Sg2000ADC : public sensor::Sensor, public PollingComponent {
 public:
  void set_channel(uint8_t channel) { channel_ = channel; }

  void setup() override {
    ESP_LOGI(TAG, "Setting up SG2000 ADC channel %u...", channel_);
    uint32_t req_mask = 0;
    if (channel_ == 1) req_mask = SMHUB_HW_ADC1;
    else if (channel_ == 2) req_mask = SMHUB_HW_ADC2;

    if (req_mask && !sg2000::smhub_hardware_is_released(req_mask)) {
      ESP_LOGE(TAG, "Hardware Arbitration FAILED! ADC channel %u is not authorized by U-Boot.", channel_);
      this->mark_failed();
      return;
    }

    if (channel_ == 1) {
      sg2000::pinmux_config(FMUX_GPIO_FUNCSEL_ADC1, 0); // ADC1 function is 0
      sg2000::pad_config(FMUX_GPIO_FUNCSEL_ADC1, false, false, 0); // No pull-up/down for ADC
    } else if (channel_ == 2) {
      sg2000::pinmux_config(FMUX_GPIO_FUNCSEL_ADC2, 0);
      sg2000::pad_config(FMUX_GPIO_FUNCSEL_ADC2, false, false, 0);
    } else if (channel_ == 3) {
      sg2000::pinmux_config(FMUX_GPIO_FUNCSEL_ADC3, 0);
      sg2000::pad_config(FMUX_GPIO_FUNCSEL_ADC3, false, false, 0);
    }
  }

  void update() override {
    if (this->is_failed()) return;
    uint32_t value;
    int ret = adc_read(channel_, &value);
    
    if (ret != 0) {
      ESP_LOGE(TAG, "Failed to read ADC channel %u (err: %d)", channel_, ret);
      this->publish_state(NAN);
      return;
    }

    // Convert raw ADC value to voltage. 
    // The SG2000 SARADC is internally tied to VDD18A (1.8V) reference
    // with 12-bit precision (0-4095).
    float voltage = (float)value / 4095.0f * 1.8f;
    this->publish_state(voltage);
  }

  void dump_config() override {
    LOG_SENSOR("", "SG2000 ADC", this);
    ESP_LOGCONFIG(TAG, "  Channel: %u", channel_);
    ESP_LOGCONFIG(TAG, "  Reference Voltage: 1.80V (Hardcoded)");
  }

 protected:
  uint8_t channel_{1};
};

}  // namespace sg2000_adc
}  // namespace esphome
