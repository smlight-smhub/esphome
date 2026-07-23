/*
 * Copyright 2026 SMLIGHT
 */
#pragma once

#include "esphome/core/preference_backend.h"

namespace esphome {
namespace sg2000 {

class SG2000Preferences : public PreferencesMixin<SG2000Preferences> {
 public:
  using PreferencesMixin<SG2000Preferences>::make_preference;

  ESPPreferenceObject make_preference(size_t length, uint32_t type, bool in_flash) {
    return ESPPreferenceObject(new SG2000PreferenceBackend(type));
  }
  ESPPreferenceObject make_preference(size_t length, uint32_t type) {
    return ESPPreferenceObject(new SG2000PreferenceBackend(type));
  }
  bool sync() { return true; }
  bool reset() { return true; }
};

void setup_preferences();

}  // namespace sg2000

}  // namespace esphome

DECLARE_PREFERENCE_ALIASES(esphome::sg2000::SG2000Preferences)
