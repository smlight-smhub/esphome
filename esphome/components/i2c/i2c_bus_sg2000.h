/*
 * Copyright (C) 2026 SMLIGHT
 * All rights reserved.
 *
 * This file is part of the SMHUB RTOS project.
 */
#pragma once

#include "esphome.h"
#include "esphome/components/i2c/i2c_bus.h"

extern "C" {
#include <i2c.h>
#include <hal_pinmux.h>
#include <cv181x_pinmux.h>
#include <cv181x_reg_fmux_gpio.h>
#include <cv181x_pinlist_swconfig.h>
}
#include "esphome/components/sg2000/sg2000_arbitration.h"

#include "esphome/core/log.h"

namespace esphome {
namespace i2c {

static const char *const TAG = "i2c.sg2000";

class Sg2000I2CBus : public i2c::I2CBus, public Component {
 protected:
  uint8_t i2c_id_;
  uint32_t frequency_{400000};  // Default 400kHz
  std::string sda_pin_name_{""};
  std::string scl_pin_name_{""};
  bool initialized_{false};

  uint32_t sda_reg_{0}, sda_func_{0};
  uint32_t scl_reg_{0}, scl_func_{0};

 public:
  Sg2000I2CBus() : i2c_id_(4) {}  // Default to I2C4

  void set_i2c_id(uint8_t i2c_id) { i2c_id_ = i2c_id; }
  void set_sda_pin(const std::string &pin) { sda_pin_name_ = pin; }
  void set_scl_pin(const std::string &pin) { scl_pin_name_ = pin; }
  void set_frequency(uint32_t frequency) {
    frequency_ = frequency;
    if (initialized_) {
      i2c_set_frequency(i2c_id_, frequency_);
    }
  }
  void set_scan(bool scan) { this->scan_ = scan; }

  bool is_scanning_ = false;

  void setup() override {
    ESP_LOGI(TAG, "Initializing SG2000 I2C Hardware Bus...");

    if (sda_pin_name_ == "SDA") {
      sda_pin_name_ = (i2c_id_ == 2) ? "VIVO_D7" : "VIVO_D0";
    }
    if (scl_pin_name_ == "SCL") {
      scl_pin_name_ = (i2c_id_ == 2) ? "VIVO_D8" : "VIVO_D1";
    }

    // Hardware Pin Mux Mapping Table
    if (sda_pin_name_ == "VIVO_D0" && i2c_id_ == 4) {
      sda_reg_ = FMUX_GPIO_FUNCSEL_VIVO_D0;
      sda_func_ = VIVO_D0__IIC4_SDA;
    } else if (sda_pin_name_ == "VIVO_D7" && i2c_id_ == 2) {
      sda_reg_ = FMUX_GPIO_FUNCSEL_VIVO_D7;
      sda_func_ = VIVO_D7__IIC2_SDA;
    } else if (sda_pin_name_ == "PAD_MIPI_TXM1" && i2c_id_ == 2) {
      sda_reg_ = FMUX_GPIO_FUNCSEL_PAD_MIPI_TXM1;
      sda_func_ = PAD_MIPI_TXM1__IIC2_SDA;
    }

    if (scl_pin_name_ == "VIVO_D1" && i2c_id_ == 4) {
      scl_reg_ = FMUX_GPIO_FUNCSEL_VIVO_D1;
      scl_func_ = VIVO_D1__IIC4_SCL;
    } else if (scl_pin_name_ == "VIVO_D8" && i2c_id_ == 2) {
      scl_reg_ = FMUX_GPIO_FUNCSEL_VIVO_D8;
      scl_func_ = VIVO_D8__IIC2_SCL;
    } else if (scl_pin_name_ == "PAD_MIPI_TXP1" && i2c_id_ == 2) {
      scl_reg_ = FMUX_GPIO_FUNCSEL_PAD_MIPI_TXP1;
      scl_func_ = PAD_MIPI_TXP1__IIC2_SCL;
    }

    if (sda_reg_ == 0 || scl_reg_ == 0) {
      ESP_LOGE(TAG, "Invalid Pin Mux configuration for I2C%u! SDA=%s, SCL=%s", i2c_id_, sda_pin_name_.c_str(),
               scl_pin_name_.c_str());
      this->mark_failed();
      return;
    }

    uint32_t hw_bit = 0;
    if (i2c_id_ == 2)
      hw_bit = sg2000::SMHUB_HW_I2C2;
    else if (i2c_id_ == 4)
      hw_bit = sg2000::SMHUB_HW_I2C4;

    if (hw_bit != 0 && !sg2000::smhub_hardware_is_released(hw_bit)) {
      ESP_LOGE(TAG, "Hardware Arbitration FAILED! I2C%u is not authorized by Linux/U-Boot!", i2c_id_);
      this->mark_failed();
      return;
    }

    sg2000::pinmux_config(sda_reg_, sda_func_);
    sg2000::pad_config(sda_reg_, true, false, 3);  // Pull-up for SDA

    sg2000::pinmux_config(scl_reg_, scl_func_);
    sg2000::pad_config(scl_reg_, true, false, 3);  // Pull-up for SCL

    i2c_init(i2c_id_);
    i2c_set_frequency(i2c_id_, frequency_);
    initialized_ = true;

    if (this->scan_) {
      this->is_scanning_ = true;
      this->i2c_scan_();
      this->is_scanning_ = false;
    }
  }

  i2c::ErrorCode write_readv(uint8_t address, const uint8_t *write_buffer, size_t write_count, uint8_t *read_buffer,
                             size_t read_count) override {
    if (!initialized_)
      return i2c::ERROR_NOT_INITIALIZED;

    if (write_count == 0 && read_count == 0) {
      if (this->is_scanning_) {
        // Designware I2C cannot do 0-byte writes.
        // For the scanner, we do a 1-byte read. Some sensors will NACK this,
        // but it is the safest way to scan without crashing sensors like the SHT4x
        // (which crashes if you send a 1-byte write of 0x00).
        static uint8_t dummy = 0;
        struct i2c_msg msg;
        msg.addr = address;
        msg.flags = I2C_M_RD;
        msg.len = 1;
        msg.buf = &dummy;
        int ret = i2c_xfer(i2c_id_, &msg, 1);
        if (ret != 0)
          return i2c::ERROR_NOT_ACKNOWLEDGED;
        return i2c::ERROR_OK;
      } else {
        // This is a 0-byte ping from a component's setup() function.
        // Since we can't do 0-byte writes on DW I2C, and 1-byte reads/writes
        // cause false NACKs or sensor crashes, we just blindly return OK.
        // The component will immediately follow up with a real transaction
        // (e.g. read_serial_number) which will naturally fail if the sensor is missing.
        return i2c::ERROR_OK;
      }
    }

    struct i2c_msg msgs[2];
    int msg_count = 0;

    if (write_count > 0) {
      msgs[msg_count].addr = address;
      msgs[msg_count].flags = 0;  // write
      msgs[msg_count].len = write_count;
      msgs[msg_count].buf = (uint8_t *) write_buffer;
      msg_count++;
    }

    if (read_count > 0) {
      msgs[msg_count].addr = address;
      msgs[msg_count].flags = I2C_M_RD;
      msgs[msg_count].len = read_count;
      msgs[msg_count].buf = read_buffer;
      msg_count++;
    }

    int ret = i2c_xfer(i2c_id_, msgs, msg_count);
    if (ret != 0) {
      if (!this->is_scanning_) {
        ESP_LOGE(TAG, "I2C Hardware Error: TX_ABRT_SOURCE = 0x%08X", ret);
      }
      return i2c::ERROR_UNKNOWN;
    }

    return i2c::ERROR_OK;
  }
};

}  // namespace i2c
}  // namespace esphome
