/*
 * Copyright 2026 SMLIGHT
 */
#include "time.h"
#include "esphome/core/log.h"
#include "smhub_ipc.h"

namespace esphome {
namespace smhub_time {

static const char *const TAG = "smhub_time";

SmhubTime *global_smhub_time = nullptr;

void SmhubTime::setup() { global_smhub_time = this; }

void SmhubTime::update() {
  smhub_hal_rpc_RpcCommand cmd = smhub_hal_rpc_RpcCommand_init_zero;
  cmd.type = smhub_hal_rpc_CommandType_GET_TIME_REQ;

  smhub_ipc_send_rpc(&cmd);
}

void SmhubTime::dump_config() { ESP_LOGCONFIG(TAG, "SMHUB Time:"); }

void SmhubTime::handle_time_response(uint64_t epoch_seconds) { this->synchronize_epoch_((uint32_t) epoch_seconds); }

extern "C" void smhub_ipc_time_cb(uint64_t epoch_seconds) {
  if (global_smhub_time != nullptr) {
    global_smhub_time->handle_time_response(epoch_seconds);
  }
}

}  // namespace smhub_time
}  // namespace esphome
