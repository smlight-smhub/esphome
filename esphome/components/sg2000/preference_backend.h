/*
 * Copyright 2026 SMLIGHT
 */
#pragma once
#ifdef USE_SG2000

#include <cstddef>
#include <cstdint>

namespace esphome::sg2000 {

class SG2000PreferenceBackend final {
 public:
  SG2000PreferenceBackend(uint32_t type) : type_(type) {}

  bool save(const uint8_t *data, size_t len);
  bool load(uint8_t *data, size_t len);

 protected:
  uint32_t type_;
};

class SG2000Preferences;

}  // namespace esphome::sg2000

namespace esphome {
using PreferenceBackend = sg2000::SG2000PreferenceBackend;
}  // namespace esphome

#endif  // USE_SG2000
