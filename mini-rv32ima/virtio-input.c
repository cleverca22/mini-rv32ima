#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "virtio.h"

uint32_t virtio_input_config_load(struct virtio_device *dev, uint32_t offset) {
  printf("input cfg ld 0x%x\n", offset);
  uint32_t ret = 0;

  switch (offset) {
  case 2: // size
    switch (dev->config_select) {
    case 1: // id name
      ret = strlen("fat-rv32ima");
      break;
    case 0x11:
      // config_subsel is a type like EV_KEY
      // value returned at 0x108, should be a bitmap, of every code (KEY_C or REL_X) the device supports
    case 0x12:
      // absolute pointer space
      // linux will only read it, if the above bitfield reports a EV_ABS type
      // it will then check for each bit (such as 1<<ABS_X), and query the range for that axis
    default:
      printf("size read for unsupported 0x%x.0x%x\n", dev->config_select, dev->config_subsel);
      break;
    }
    break;
  default:
    if (offset >= 8) {
      switch (dev->config_select) {
      case 1: // id name
        const char *str = "fat-rv32ima";
        ret = str[offset - 8];
        break;
      }
    }
  }
  return ret;
}

void virtio_input_config_store(struct virtio_device *dev, uint32_t offset, uint32_t val) {
  switch (offset) {
  case 0: // virtio-input config select
    dev->config_select = val;
    break;
  case 1: // virtio-input config sub select
    dev->config_subsel = val;
    break;
  }
}
const virtio_device_type virtio_input_type = {
  .device_type = 18,
  .queue_count = 2,
  .config_load = virtio_input_config_load,
  .config_store = virtio_input_config_store,
};
