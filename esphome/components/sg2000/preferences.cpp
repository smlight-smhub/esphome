/*
 * Copyright 2026 SMLIGHT
 */
#include "preferences.h"
#include "esphome/core/log.h"
#include "smhub_ipc.h"
#include <cstring>
#include "FreeRTOS.h"
#include "task.h"

namespace esphome {

extern "C" void rpmsg_process_queue(void);

ESPPreferences *global_preferences = nullptr;

namespace sg2000 {

static const char *const TAG = "sg2000.preferences";

static volatile bool pref_load_ready = false;
static std::vector<uint8_t> pref_load_data;
static uint32_t pref_load_target_hash = 0;

extern "C" void smhub_ipc_pref_load_cb(uint32_t hash, const uint8_t *data, size_t len) {
  if (hash == pref_load_target_hash) {
    pref_load_data.assign(data, data + len);
    pref_load_ready = true;
    ESP_LOGV(TAG, "CB Received pref_load_resp for hash %lu, len %d, data: %02X", (unsigned long) hash, (int) len,
             len > 0 ? data[0] : 0);
  }
}

bool SG2000PreferenceBackend::save(const uint8_t *data, size_t len) {
  smhub_hal_rpc_RpcCommand cmd = smhub_hal_rpc_RpcCommand_init_zero;
  cmd.type = smhub_hal_rpc_CommandType_PREF_SAVE_REQ;
  cmd.has_pref_save_req = true;
  cmd.pref_save_req.hash = this->type_;

  // limit len to max nanopb bytes size (256)
  if (len > sizeof(cmd.pref_save_req.data.bytes)) {
    ESP_LOGE(TAG, "Preference data too large: %d", (int) len);
    return false;
  }

  cmd.pref_save_req.data.size = len;
  if (len > 0) {
    std::memcpy(cmd.pref_save_req.data.bytes, data, len);
  }

  ESP_LOGV(TAG, "Saving preference hash %lu, len %d, data: %02X", (unsigned long) this->type_, (int) len,
           len > 0 ? data[0] : 0);
  smhub_ipc_send_rpc(&cmd);
  return true;
}

bool SG2000PreferenceBackend::load(uint8_t *data, size_t len) {
  pref_load_target_hash = this->type_;
  pref_load_ready = false;
  pref_load_data.clear();

  smhub_hal_rpc_RpcCommand cmd = smhub_hal_rpc_RpcCommand_init_zero;
  cmd.type = smhub_hal_rpc_CommandType_PREF_LOAD_REQ;
  cmd.has_pref_load_req = true;
  cmd.pref_load_req.hash = this->type_;

  ESP_LOGV(TAG, "Requesting preference load for hash %lu, expected len %d", (unsigned long) this->type_, (int) len);
  smhub_ipc_send_rpc(&cmd);

  // Spin wait for response. The VirtIO ISR flags data, but we must poll it.
  // Call rpmsg_process_queue() to process incoming PREF_LOAD_RESP.
  int timeout_ms = 500;
  while (!pref_load_ready && timeout_ms > 0) {
    rpmsg_process_queue();
    vTaskDelay(pdMS_TO_TICKS(10));
    timeout_ms -= 10;
  }

  if (!pref_load_ready) {
    ESP_LOGE(TAG, "Preference load timeout for hash %lu", (unsigned long) this->type_);
    return false;
  }

  if (pref_load_data.empty()) {
    ESP_LOGV(TAG, "Preference load returned empty for hash %lu", (unsigned long) this->type_);
    return false;  // not found
  }

  if (pref_load_data.size() != len) {
    ESP_LOGW(TAG, "Preference size mismatch: expected %d, got %d", (int) len, (int) pref_load_data.size());
    return false;  // size mismatch
  }

  std::memcpy(data, pref_load_data.data(), len);
  ESP_LOGV(TAG, "Preference loaded successfully for hash %lu, data: %02X", (unsigned long) this->type_,
           len > 0 ? data[0] : 0);
  return true;
}

void setup_preferences() { global_preferences = new SG2000Preferences(); }

}  // namespace sg2000
}  // namespace esphome
