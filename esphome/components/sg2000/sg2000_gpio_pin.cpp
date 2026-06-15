/*
 * Copyright 2026 SMLIGHT
 */
#include "esphome/components/sg2000/sg2000_gpio_pin.h"
#include <map>
#include <vector>
#include <vector>
#include "smhub_ipc.h"
#include <cstring>
namespace esphome {
namespace sg2000 {

static const char *const TAG_GPIO = "sg2000.gpio";
static std::map<std::string, Sg2000InternalGPIOPin*> pin_registry;
static std::map<std::string, InterruptHandler> interrupt_registry;

void register_pin(Sg2000InternalGPIOPin* pin, const std::string &pin_name) {
  pin_registry[pin_name] = pin;
}

void register_interrupt(const std::string &pin_name, void (*func)(void *), void *arg, gpio::InterruptType type) {
  InterruptHandler handler = {func, arg, type};
  interrupt_registry[pin_name] = handler;
}

void trigger_interrupt(const std::string &pin_name, bool new_state) {
  auto pin_it = pin_registry.find(pin_name);
  if (pin_it != pin_registry.end()) {
    pin_it->second->set_state(new_state);
  }

  auto it = interrupt_registry.find(pin_name);
  if (it != interrupt_registry.end()) {
    bool trigger = false;
    if (it->second.type == gpio::INTERRUPT_ANY_EDGE) trigger = true;
    else if (it->second.type == gpio::INTERRUPT_RISING_EDGE && new_state) trigger = true;
    else if (it->second.type == gpio::INTERRUPT_FALLING_EDGE && !new_state) trigger = true;

    if (trigger && it->second.func != nullptr) {
      it->second.func(it->second.arg);
    }
  }
}

void Sg2000InternalGPIOPin::setup() {
  register_pin(this, this->pin_name_);
  this->pin_mode(this->flags_);
}

size_t Sg2000InternalGPIOPin::dump_summary(char *buffer, size_t len) const {
  return snprintf(buffer, len, "%s", this->pin_name_.c_str());
}

static void send_gpio_config(const std::string &pin_name, int32_t mode, int32_t edge, int32_t bias) {
  smhub_hal_rpc_RpcCommand cmd = smhub_hal_rpc_RpcCommand_init_zero;
  cmd.type = smhub_hal_rpc_CommandType_GPIO_CONFIG;
  cmd.has_gpio_config = true;
  strncpy(cmd.gpio_config.pin_name, pin_name.c_str(), sizeof(cmd.gpio_config.pin_name) - 1);
  cmd.gpio_config.mode = mode;
  cmd.gpio_config.edge = edge;
  cmd.gpio_config.bias = bias;
  smhub_ipc_send_rpc(&cmd);
}

void Sg2000InternalGPIOPin::pin_mode(gpio::Flags flags) {
  this->flags_ = flags;
  int32_t mode = (flags & gpio::FLAG_OUTPUT) ? 1 : 0;
  int32_t bias = 0;
  if (flags & gpio::FLAG_PULLUP) bias = 1;
  else if (flags & gpio::FLAG_PULLDOWN) bias = 2;
  send_gpio_config(this->pin_name_, mode, 0, bias);
}

void Sg2000InternalGPIOPin::attach_interrupt(void (*func)(void *), void *arg, gpio::InterruptType type) const {
  register_interrupt(this->pin_name_, func, arg, type);
  
  int32_t mode = (this->flags_ & gpio::FLAG_OUTPUT) ? 1 : 0;
  int32_t edge = 0;
  if (type == gpio::INTERRUPT_RISING_EDGE) edge = 1;
  else if (type == gpio::INTERRUPT_FALLING_EDGE) edge = 2;
  else if (type == gpio::INTERRUPT_ANY_EDGE) edge = 3;

  int32_t bias = 0;
  if (this->flags_ & gpio::FLAG_PULLUP) bias = 1;
  else if (this->flags_ & gpio::FLAG_PULLDOWN) bias = 2;

  send_gpio_config(this->pin_name_, mode, edge, bias);
}

void Sg2000InternalGPIOPin::detach_interrupt() const {
  interrupt_registry.erase(this->pin_name_);
  int32_t mode = (this->flags_ & gpio::FLAG_OUTPUT) ? 1 : 0;
  int32_t bias = 0;
  if (this->flags_ & gpio::FLAG_PULLUP) bias = 1;
  else if (this->flags_ & gpio::FLAG_PULLDOWN) bias = 2;
  send_gpio_config(this->pin_name_, mode, 0, bias);
}

bool Sg2000InternalGPIOPin::digital_read() {
  return this->inverted_ ? !this->state_ : this->state_;
}

void Sg2000InternalGPIOPin::digital_write(bool value) {
  this->state_ = value;
  bool actual_state = this->inverted_ ? !value : value;

  smhub_hal_rpc_RpcCommand cmd = smhub_hal_rpc_RpcCommand_init_zero;
  cmd.type = smhub_hal_rpc_CommandType_GPIO_TOGGLE;
  strncpy(cmd.pin_name, this->pin_name_.c_str(), sizeof(cmd.pin_name) - 1);
  cmd.state = actual_state;
  smhub_ipc_send_rpc(&cmd);
}

}  // namespace sg2000
}  // namespace esphome

extern "C" void smhub_ipc_gpio_edge_cb(const char* pin_name, bool state) {
  std::string name(pin_name);
  esphome::sg2000::trigger_interrupt(name, state);
}

namespace esphome {

bool ISRInternalGPIOPin::digital_read() {
  if (this->arg_ == nullptr) return false;
  return reinterpret_cast<esphome::sg2000::Sg2000InternalGPIOPin*>(this->arg_)->digital_read();
}

void ISRInternalGPIOPin::digital_write(bool value) {
  if (this->arg_ == nullptr) return;
  reinterpret_cast<esphome::sg2000::Sg2000InternalGPIOPin*>(this->arg_)->digital_write(value);
}

void ISRInternalGPIOPin::clear_interrupt() {
}

void ISRInternalGPIOPin::pin_mode(gpio::Flags flags) {
  if (this->arg_ == nullptr) return;
  reinterpret_cast<esphome::sg2000::Sg2000InternalGPIOPin*>(this->arg_)->pin_mode(flags);
}

}  // namespace esphome
