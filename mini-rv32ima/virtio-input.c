#include <stdint.h>
#include "virtio.h"

uint32_t virtio_input_config_load(virtio_device *dev, uint32_t offset) {
  uint32_t ret = 0;
  switch (dev->config_select) {
  case 1: // id name
    const char *str = "fat-rv32ima";
    ret = str[offset - 8];
  }
  return ret;
}

void virtio_input_config_store(struct virtio_device *dev, uint32_t offset, uint32_t val) {
}
static const virtio_device_type virtio_input_type = {
  .device_type = 18,
  .queue_count = 2,
  .config_load = virtio_input_config_load,
  .config_store = virtio_input_config_store,
};
