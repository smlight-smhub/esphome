/*
 * Copyright 2026 SMLIGHT
 */
#pragma once

#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace smhub_time {

class SmhubTime : public time::RealTimeClock {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;

  void handle_time_response(uint64_t epoch_seconds);
};

extern SmhubTime *global_smhub_time;

}  // namespace smhub_time
}  // namespace esphome
