/*
 * Copyright (C) 2026 SMLIGHT
 * All rights reserved.
 *
 * This file is part of the SMHUB RTOS project.
 */
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/output/float_output.h"
#include "esphome/core/log.h"

extern "C" {
#include "pwm.h"
#include "hal_pinmux.h"
#include "cv181x_reg_fmux_gpio.h"
#include "cv181x_pinlist_swconfig.h"
}
#include "esphome/components/sg2000/sg2000_arbitration.h"

namespace esphome {
namespace sg2000_pwm {

static const char *const TAG = "sg2000_pwm";

class Sg2000PWM : public output::FloatOutput, public Component {
 public:
  void set_pwm_id(uint8_t pwm_id) { pwm_id_ = pwm_id; }
  void set_channel(uint8_t channel) { channel_ = channel; }
  void set_frequency(float frequency) { frequency_ = frequency; }
  void set_pin_mux(uint32_t reg, uint32_t func) {
    pin_reg_ = reg;
    pin_func_ = func;
  }

  void setup() override {
    ESP_LOGI(TAG, "Setting up SG2000 PWM%u Channel %u at %.1f Hz...", pwm_id_, channel_, frequency_);
    uint32_t req_mask = 0;
    if (pwm_id_ == 0)
      req_mask = sg2000::SMHUB_HW_PWM0;
    else if (pwm_id_ == 1)
      req_mask = sg2000::SMHUB_HW_PWM1;
    else if (pwm_id_ == 2)
      req_mask = sg2000::SMHUB_HW_PWM2;
    else if (pwm_id_ == 3)
      req_mask = sg2000::SMHUB_HW_PWM3;

    if (req_mask && !sg2000::smhub_hardware_is_released(req_mask)) {
      ESP_LOGW(TAG, "Hardware Arbitration FAILED! PWM%u is not authorized by U-Boot,", pwm_id_);
      this->mark_failed();
      return;
    }

    sg2000::pinmux_config(pin_reg_, pin_func_);
    sg2000::pad_config(pin_reg_, false, false, 3);

    pwm_init_channel(pwm_id_, channel_);
    this->turn_off();
  }

  void dump_config() override {
    ESP_LOGCONFIG(TAG, "SG2000 PWM:");
    ESP_LOGCONFIG(TAG, "  PWM ID: %u", pwm_id_);
    ESP_LOGCONFIG(TAG, "  Channel: %u", channel_);
    ESP_LOGCONFIG(TAG, "  Frequency: %.1f Hz", frequency_);
  }

  void update_frequency(float frequency) override { frequency_ = frequency; }

 protected:
  void write_state(float state) override {
    if (this->is_failed())
      return;
    // state is duty cycle 0.0 to 1.0
    // period in nanoseconds
    uint32_t period_ns = (uint32_t) (1000000000.0f / frequency_);
    uint32_t duty_ns = (uint32_t) (period_ns * state);
    bool enable = state > 0.001f;

    pwm_set_state(pwm_id_, channel_, period_ns, duty_ns, enable);
  }

  uint8_t pwm_id_{0};
  uint8_t channel_{0};
  float frequency_{1000.0f};  // Default 1kHz
  uint32_t pin_reg_{0};
  uint32_t pin_func_{0};
};

}  // namespace sg2000_pwm
}  // namespace esphome
