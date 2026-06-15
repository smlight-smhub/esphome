/*
 * Copyright 2026 SMLIGHT
 */
#pragma once

#include "esphome/core/gpio.h"
#include <string>

namespace esphome {
namespace sg2000 {

class Sg2000InternalGPIOPin : public InternalGPIOPin {
 protected:
  std::string pin_name_;

 public:
  void set_pin_name(const std::string &name) { pin_name_ = name; }
  const std::string &get_pin_name() const { return pin_name_; }
  void set_inverted(bool inverted) { inverted_ = inverted; }
  void set_flags(gpio::Flags flags) { flags_ = flags; }

  void setup() override;
  void pin_mode(gpio::Flags flags) override;
  gpio::Flags get_flags() const override { return flags_; }
  bool digital_read() override;
  void digital_write(bool value) override;
  void detach_interrupt() const override;
  ISRInternalGPIOPin to_isr() const override { return {const_cast<Sg2000InternalGPIOPin*>(this)}; }
  uint8_t get_pin() const override { return 0; }
  size_t dump_summary(char *buffer, size_t len) const override;
  bool is_inverted() const override { return inverted_; }
  void set_state(bool state) { state_ = state; }

 protected:
  void attach_interrupt(void (*func)(void *), void *arg, gpio::InterruptType type) const override;

  bool inverted_{false};
  gpio::Flags flags_{gpio::FLAG_OUTPUT};
  bool state_{false};
};

// Static registry for interrupt callbacks
struct InterruptHandler {
  void (*func)(void *);
  void *arg;
  gpio::InterruptType type;
};
void register_interrupt(const std::string &pin_name, void (*func)(void *), void *arg, gpio::InterruptType type);
void trigger_interrupt(const std::string &pin_name, bool new_state);

}  // namespace sg2000
}  // namespace esphome
