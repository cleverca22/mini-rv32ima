#include <stdint.h>
#include <stdio.h>
#include "virtio.h"

uint32_t virtio_blk_config_load(virtio_device *dev, uint32_t offset) {
  uint32_t ret = 0;
  printf("virtio_blk_config_load(%p, %d)\n", dev, offset);
  switch (offset) {
  case 0:
    ret = 1;
    break;
  }
  return ret;
}

void virtio_blk_config_store(struct virtio_device *dev, uint32_t offset, uint32_t val) {
}


const virtio_device_type virtio_blk_type = {
  .device_type = 2,
  .queue_count = 1,
  .config_load = virtio_blk_config_load,
  .config_store = virtio_blk_config_store,
};
