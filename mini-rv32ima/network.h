#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef void (*rx_callback)(uint8_t *packet, uint32_t size);

bool network_init(rx_callback callback);
void network_teardown(void);
void network_transmit(uint8_t *packet, uint32_t size);

int network_get_fd(void);
