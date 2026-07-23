/*
 * Copyright 2026 SMLIGHT
 */
#include "socket.h"
#include "rpmsg_sockets_impl.h"
#include "esphome/core/log.h"
#include <cerrno>

#ifdef USE_SOCKET_IMPL_RPMSG_SOCKETS

// Declared in rpmsg_app.c
extern "C" void esphome_rpmsg_tx(const uint8_t *data, size_t len);

extern "C" uint8_t connection_established = 0;

#include "FreeRTOS.h"
#include "semphr.h"
#include <vector>
#include <queue>
#include <algorithm>

namespace esphome::socket {

static const char *const TAG = "socket.rpmsg";

static SemaphoreHandle_t sockets_mutex = NULL;
static std::vector<RPMSGSocketImpl *> active_sockets;
static std::queue<uint32_t> pending_connections;
static std::vector<std::unique_ptr<RPMSGSocketImpl>> active_client_owners;

static void init_sockets_mutex() {
  if (sockets_mutex == NULL) {
    sockets_mutex = xSemaphoreCreateMutex();
  }
}

static void lock_sockets() {
  init_sockets_mutex();
  xSemaphoreTake(sockets_mutex, portMAX_DELAY);
}

static void unlock_sockets() { xSemaphoreGive(sockets_mutex); }

extern "C" void esphome_rpmsg_rx_cb(const uint8_t *data, size_t len) {
  if (!connection_established && len > 0) {
    connection_established = true;
  }

  if (len < 4)
    return;

  uint32_t conn_id;
  memcpy(&conn_id, data, 4);

  lock_sockets();
  auto it = std::find_if(active_sockets.begin(), active_sockets.end(),
                         [conn_id](RPMSGSocketImpl *s) { return s->get_conn_id() == conn_id; });

  if (len == 4) {
    // Zero-payload EOF / close event
    if (it != active_sockets.end()) {
      (*it)->mark_closed();
    }
  } else {
    // Data packet
    if (it != active_sockets.end() && !(*it)->is_closed()) {
      (*it)->push_rx_data(data + 4, len - 4);
    } else {
      // Reused conn_id (race condition reconnect): evict the stale closed socket
      if (it != active_sockets.end()) {
        active_sockets.erase(it);
        active_client_owners.erase(std::remove_if(active_client_owners.begin(), active_client_owners.end(),
                                                  [conn_id](const std::unique_ptr<RPMSGSocketImpl> &s) {
                                                    return s->get_conn_id() == conn_id;
                                                  }),
                                   active_client_owners.end());
      }

      // Lazy accept: new connection
      auto client = make_unique<RPMSGSocketImpl>(1);
      client->set_conn_id(conn_id);
      active_sockets.push_back(client.get());
      client->push_rx_data(data + 4, len - 4);
      active_client_owners.push_back(std::move(client));
      pending_connections.push(conn_id);
    }
  }
  unlock_sockets();
}

extern "C" void esphome_rpmsg_sync_config(void) __attribute__((weak));

extern "C" void esphome_rpmsg_reset() {
  connection_established = 0;

  std::vector<std::unique_ptr<RPMSGSocketImpl>> temp_owners;

  lock_sockets();
  for (auto *s : active_sockets) {
    s->mark_closed();
  }
  while (!pending_connections.empty()) {
    pending_connections.pop();
  }
  temp_owners = std::move(active_client_owners);
  unlock_sockets();

  if (esphome_rpmsg_sync_config) {
    esphome_rpmsg_sync_config();
  }
}

RPMSGSocketImpl::RPMSGSocketImpl(int fd) : fd_(fd) {
  if (fd_ == 0) {
    is_server_ = true;
  }
}

RPMSGSocketImpl::~RPMSGSocketImpl() { close(); }

int RPMSGSocketImpl::close() {
  if (fd_ == -1)
    return 0;

  lock_sockets();
  active_sockets.erase(std::remove(active_sockets.begin(), active_sockets.end(), this), active_sockets.end());
  active_client_owners.erase(
      std::remove_if(active_client_owners.begin(), active_client_owners.end(),
                     [this](const std::unique_ptr<RPMSGSocketImpl> &s) { return s.get() == this; }),
      active_client_owners.end());
  unlock_sockets();

  if (!is_server_ && !is_closed_) {
    uint32_t header = conn_id_;
    uint8_t buf[4];
    memcpy(buf, &header, 4);
    esphome_rpmsg_tx(buf, 4);
  }

  fd_ = -1;
  is_closed_ = true;
  return 0;
}

std::unique_ptr<RPMSGSocketImpl> RPMSGSocketImpl::accept(struct sockaddr *addr, socklen_t *addrlen) {
  if (!is_server_)
    return nullptr;

  uint32_t next_conn_id = 0;
  lock_sockets();
  if (!pending_connections.empty()) {
    next_conn_id = pending_connections.front();
    pending_connections.pop();
  }
  unlock_sockets();

  if (next_conn_id != 0) {
    lock_sockets();
    auto it = std::find_if(
        active_client_owners.begin(), active_client_owners.end(),
        [next_conn_id](const std::unique_ptr<RPMSGSocketImpl> &s) { return s->get_conn_id() == next_conn_id; });
    if (it != active_client_owners.end()) {
      std::unique_ptr<RPMSGSocketImpl> client = std::move(*it);
      active_client_owners.erase(it);
      unlock_sockets();
      return client;
    }
    unlock_sockets();
  }
  return nullptr;
}

bool RPMSGSocketImpl::ready() const {
  if (is_server_) {
    lock_sockets();
    bool has_pending = !pending_connections.empty();
    unlock_sockets();
    return has_pending;
  }
  return is_closed_ || (rx_head_ != rx_tail_);
}

ssize_t RPMSGSocketImpl::read(void *buf, size_t len) {
  if (is_closed_ && rx_head_ == rx_tail_) {
    return 0;  // EOF
  }

  uint8_t *dest = static_cast<uint8_t *>(buf);
  size_t bytes_read = 0;
  while (bytes_read < len && rx_head_ != rx_tail_) {
    dest[bytes_read++] = rx_buffer_[rx_tail_];
    rx_tail_ = (rx_tail_ + 1) % RX_BUFFER_SIZE;
  }

  if (bytes_read == 0) {
    errno = EWOULDBLOCK;
    return -1;
  }
  return bytes_read;
}

void RPMSGSocketImpl::push_rx_data(const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    size_t next_head = (rx_head_ + 1) % RX_BUFFER_SIZE;
    if (next_head == rx_tail_) {
      break;  // Buffer overflow
    }
    rx_buffer_[rx_head_] = data[i];
    rx_head_ = next_head;
  }
}

ssize_t RPMSGSocketImpl::write(const void *buf, size_t len) {
  if (is_closed_) {
    errno = EPIPE;
    return -1;
  }

  const uint8_t *data = static_cast<const uint8_t *>(buf);
  size_t remaining = len;
  size_t offset = 0;

  uint8_t tx_buf[512];
  uint32_t header = conn_id_;
  memcpy(tx_buf, &header, 4);

  while (remaining > 0) {
    size_t chunk_size = (remaining > 492) ? 492 : remaining;
    memcpy(tx_buf + 4, data + offset, chunk_size);

    esphome_rpmsg_tx(tx_buf, chunk_size + 4);

    offset += chunk_size;
    remaining -= chunk_size;
  }

  return len;
}

ssize_t RPMSGSocketImpl::readv(const struct iovec *iov, int iovcnt) {
  ssize_t total = 0;
  for (int i = 0; i < iovcnt; i++) {
    ssize_t res = read(iov[i].iov_base, iov[i].iov_len);
    if (res > 0) {
      total += res;
      if ((size_t) res < iov[i].iov_len)
        break;
    } else if (res < 0 && total == 0) {
      return -1;
    } else {
      break;
    }
  }
  return total;
}

ssize_t RPMSGSocketImpl::writev(const struct iovec *iov, int iovcnt) {
  ssize_t total = 0;
  for (int i = 0; i < iovcnt; i++) {
    ssize_t res = write(iov[i].iov_base, iov[i].iov_len);
    if (res > 0) {
      total += res;
    } else if (res < 0 && total == 0) {
      return -1;
    } else {
      break;
    }
  }
  return total;
}

std::unique_ptr<Socket> socket(int domain, int type, int protocol) { return make_unique<RPMSGSocketImpl>(0); }

std::unique_ptr<Socket> socket_loop_monitored(int domain, int type, int protocol) {
  return make_unique<RPMSGSocketImpl>(0);
}

}  // namespace esphome::socket

#endif  // USE_SOCKET_IMPL_RPMSG_SOCKETS
