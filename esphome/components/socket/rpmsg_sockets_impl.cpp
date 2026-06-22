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

namespace esphome::socket {

static const char *const TAG = "socket.rpmsg";
static RPMSGSocketImpl *server_socket = nullptr;

// A simple static ring buffer for incoming data from Linux over RPMSG
static constexpr size_t RX_BUFFER_SIZE = 8192;
static uint8_t rx_buffer[RX_BUFFER_SIZE];
static size_t rx_head = 0;
static size_t rx_tail = 0;



extern "C" void esphome_rpmsg_rx_cb(const uint8_t *data, size_t len) {
    if (!connection_established && len > 0) {
        connection_established = true;
    }

    for (size_t i = 0; i < len; i++) {
        size_t next_head = (rx_head + 1) % RX_BUFFER_SIZE;
        if (next_head == rx_tail) {
            ESP_LOGW(TAG, "RPMSG RX buffer overflow!");
            break;
        }
        rx_buffer[rx_head] = data[i];
        rx_head = next_head;
    }
}

extern "C" void esphome_rpmsg_sync_config(void) __attribute__((weak));

extern "C" void esphome_rpmsg_reset() {
    connection_established = 0;
    rx_head = 0;
    rx_tail = 0;
    if (esphome_rpmsg_sync_config) {
        esphome_rpmsg_sync_config();
    }
}

RPMSGSocketImpl::RPMSGSocketImpl(int fd) : fd_(fd) {
    if (fd_ == 0) {
        server_socket = this;
    }
}

RPMSGSocketImpl::~RPMSGSocketImpl() {
    if (fd_ == 0 && server_socket == this) {
        server_socket = nullptr;
    } else if (fd_ == 1) {
        esphome_rpmsg_reset();
        if (server_socket != nullptr) {
            server_socket->fd_ = 0;
        }
    }
}

std::unique_ptr<RPMSGSocketImpl> RPMSGSocketImpl::accept(struct sockaddr *addr, socklen_t *addrlen) {
    if (fd_ == 0 && connection_established) {
        // Return the client socket. We change our fd so we don't accept again until closed.
        fd_ = -1; 
        return make_unique<RPMSGSocketImpl>(1);
    }
    return nullptr;
}

bool RPMSGSocketImpl::ready() const {
    return rx_head != rx_tail;
}

ssize_t RPMSGSocketImpl::read(void *buf, size_t len) {
    if (fd_ == 1 && !connection_established) {
        return 0;
    }
    uint8_t *dest = static_cast<uint8_t *>(buf);
    size_t bytes_read = 0;
    while (bytes_read < len && rx_head != rx_tail) {
        dest[bytes_read++] = rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;
    }
    if (bytes_read == 0) {
        errno = EWOULDBLOCK;
        return -1;
    }
    return bytes_read;
}

ssize_t RPMSGSocketImpl::readv(const struct iovec *iov, int iovcnt) {
    ssize_t total = 0;
    for (int i = 0; i < iovcnt; i++) {
        ssize_t res = read(iov[i].iov_base, iov[i].iov_len);
        if (res > 0) {
            total += res;
            if ((size_t)res < iov[i].iov_len) break;
        } else if (res < 0 && total == 0) {
            return -1;
        } else {
            break;
        }
    }
    return total;
}

ssize_t RPMSGSocketImpl::write(const void *buf, size_t len) {
    const uint8_t *data = static_cast<const uint8_t *>(buf);
    size_t remaining = len;
    size_t offset = 0;
    
    // OpenAMP rpmsg_send has a strict MTU (usually 496 bytes for a 512 byte vring buffer).
    while (remaining > 0) {
        size_t chunk_size = (remaining > 496) ? 496 : remaining;
        esphome_rpmsg_tx(data + offset, chunk_size);
        offset += chunk_size;
        remaining -= chunk_size;
    }
    
    return len;
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


std::unique_ptr<Socket> socket(int domain, int type, int protocol) {
    return make_unique<RPMSGSocketImpl>(0);
}

std::unique_ptr<Socket> socket_loop_monitored(int domain, int type, int protocol) {
    return make_unique<RPMSGSocketImpl>(0);
}

}  // namespace esphome::socket

#endif  // USE_SOCKET_IMPL_RPMSG_SOCKETS
