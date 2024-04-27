#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "virtio.h"

static virtio_device *virtio_devices[64];
static int virtio_count = 0;

static void virtio_mmio_store(virtio_device *dev, uint32_t offset, uint32_t val);
static uint32_t virtio_mmio_load(virtio_device *dev, uint32_t offset);

// i think virtio_mmio in linux, automatically jams the entire queue full with requests for virtio_input_event objects
// so it has guest ram assigned for every event it can receive at once
virtio_device *virtio_create(void *ram_image, const virtio_device_type *type, uint32_t base, uint32_t size) {
  int queues = type->queue_count;
  virtio_device *dev = malloc(sizeof(virtio_device));
  bzero(dev, sizeof(virtio_device));
  dev->index = virtio_count;
  dev->type = type;
  dev->reg_base = base;
  dev->reg_size = size;
  dev->ram_image = ram_image;
  dev->queues = malloc(sizeof(virtio_queue) * queues);
  dev->queue_count = queues;
  virtio_devices[virtio_count] = dev;
  virtio_count++;
  return dev;
}

static uint16_t virtio_ld16(virtio_device *dev, uint32_t addy) {
  addy -= 0x80000000;
  return *((uint16_t*)(dev->ram_image + addy));
}

static uint16_t virtio_avail_ld16(virtio_device *dev, int queue, uint32_t offset) {
  return virtio_ld16(dev, offset + dev->queues[queue].QueueDriverLow);
}

static void hexdump_ram(void *ram_image, uint32_t addr, uint32_t len) {
  for (int i=0; i<len; i++) {
    uint32_t addy = addr + i;
    if ((i % 16) == 0) printf("0x%x: ", addy);

    uint8_t t = *((uint8_t*)(ram_image + (addy - 0x80000000)));
    printf("%02x ", t);
    if ((i % 16) == 15) printf("\n");
  }
  printf("\n");
}

static void virtio_dump_desc(virtio_device *dev, const virtio_desc *desc) {
  printf("\taddr: 0x%lx\n", desc->addr);
  printf("\tlen: %d\n", desc->len);
  printf("\tflags: 0x%x\n", desc->flags);
  printf("\tnext: %d\n", desc->next);
  hexdump_ram(dev->ram_image, desc->addr, desc->len);
}

static void virtio_dump_available(virtio_device *dev, int queue) {
  printf("dumping available queue %d\n", queue);
  printf("base addr: 0x%x\n", dev->queues[queue].QueueDriverLow);
  printf("flags: 0x%x\n", virtio_avail_ld16(dev, queue, 0));
  uint16_t ring_idx = virtio_avail_ld16(dev, queue, 2);
  printf("idx: 0x%x\n", ring_idx);
  hexdump_ram(dev->ram_image, dev->queues[queue].QueueDescLow, 256);
  for (int n=0; n<dev->queues[queue].QueueNum; n++) {
    if (n == (ring_idx % dev->queues[queue].QueueNum)) {
      puts("hit write ptr");
      break;
    }
    uint16_t idx = virtio_avail_ld16(dev, queue, 2 + (n*2));
    uint16_t idx_capped = idx % dev->queues[queue].QueueNum;
    printf("slot[%d]: %d(%d)\n", n, idx, idx_capped);
    if ((idx > 0) && (idx < 64)) {
      virtio_desc *desc = dev->queues[queue].QueueDescLow - 0x80000000 + ((idx-1) * 16) + dev->ram_image;
again:
      virtio_dump_desc(dev, desc);
      if (desc->flags & 1) {
        idx = desc->next;
        desc = dev->queues[queue].QueueDescLow - 0x80000000 + (idx * 16) + dev->ram_image;
        goto again;
      }
    }
  }
  // VIRTIO_F_EVENT_IDX(29)
  printf("used_event: 0x%x\n", virtio_avail_ld16(dev, queue, 2 + (64*2)));
}

static void virtio_dump_rings(virtio_device *dev) {
  printf("virtio%d has %d queues\n", dev->index, dev->queue_count);
  for (int n=0; n<dev->queue_count; n++) {
    virtio_dump_available(dev, n);
  }
}

void virtio_dump_all() {
  for (int n=0; n<virtio_count; n++) {
    virtio_dump_rings(virtio_devices[n]);
  }
}

bool virtio_store(uint32_t addr, uint32_t val) {
  bool handled = false;
  for (int n=0; n<virtio_count; n++) {
    uint32_t base = virtio_devices[n]->reg_base;
    uint32_t size = virtio_devices[n]->reg_size;
    if ((addr >= base) && (addr < (base+size))) {
      virtio_mmio_store(virtio_devices[n], addr - base, val);
      handled = true;
      break;
    }
  }
  return handled;
}

uint32_t virtio_load(uint32_t addr) {
  uint32_t ret = 0;
  for (int n=0; n<virtio_count; n++) {
    uint32_t base = virtio_devices[n]->reg_base;
    uint32_t size = virtio_devices[n]->reg_size;
    if ((addr >= base) && (addr < (base+size))) {
      ret = virtio_mmio_load(virtio_devices[n], addr - base);
      break;
    }
  }
  return ret;
}

static void virtio_mmio_store(virtio_device *dev, uint32_t offset, uint32_t val) {
  printf("virtio%d_store(0x%x, 0x%x)\n", dev->index, offset, val);
  switch (offset) {
  case 0x14:
    dev->DeviceFeaturesSel = val;
    break;
  case 0x20: // DriverFeatures
    switch (dev->DriverFeaturesSel) {
    case 1:
      printf("feature high set to 0x%x\n", val);
      break;
    }
    break;
  case 0x24:
    dev->DriverFeaturesSel = val;
    break;
  case 0x30:
    dev->QueueSel = val;
    printf("QueueSel = %d\n", val);
    break;
  case 0x38:
    dev->queues[dev->QueueSel].QueueNum = val;
    printf("QueueNum[%d.%d] = %d\n", dev->index, dev->QueueSel, val);
    break;
  case 0x44:
    dev->queues[dev->QueueSel].QueueReady = val;
    if (val == 1) {
      printf("virtio%d_queue%d has become ready\n", dev->index, dev->QueueSel);
    }
    break;
  case 0x50: // QueueNotify
    printf("QueueNotify %d\n", val);
    virtio_dump_rings(dev);
    break;
  case 0x64: // InterruptACK
    virtio_maybe_clear_irq();
    break;
  case 0x70:
    dev->Status = val;
    if (val == 0) {
      puts("virtio reset");
    } else {
      if (val & 1) puts("VIRTIO_CONFIG_S_ACKNOWLEDGE");
      if (val & 2) puts("VIRTIO_CONFIG_S_DRIVER (driver found for dev)");
      if (val & 4) puts("VIRTIO_CONFIG_S_DRIVER_OK");
      if (val & 8) puts("VIRTIO_CONFIG_S_FEATURES_OK");
    }
    break;
  case 0x80:
    dev->queues[dev->QueueSel].QueueDescLow = val;
    break;
  case 0x84: // QueueDescHigh
    assert(val == 0);
    break;
  case 0x90:
    dev->queues[dev->QueueSel].QueueDriverLow = val;
    break;
  case 0x94: // QueueDriverHigh
    assert(val == 0);
    break;
  case 0xa0:
    dev->queues[dev->QueueSel].QueueDeviceLow = val;
    break;
  case 0xa4: // QueueDeviceHigh
    assert(val == 0);
    break;
  case 0x100: // virtio-input config select
    dev->config_select = val;
    break;
  case 0x101: // virtio-input config sub select
    dev->config_subsel = val;
    break;
  }
}

static uint32_t virtio_mmio_load(virtio_device *dev, uint32_t offset) {
  uint32_t ret = 0;
  switch (offset) {
  case 0: // MagicValue
    ret = 0x74726976;
    break;
  case 4: // Version
    ret = 2;
    break;
  case 8: // DeviceID
    ret = dev->type->device_type;
    break;
  case 0x10: // DeviceFeatures
    switch (dev->DeviceFeaturesSel) {
    case 1:
      ret = 1;
      break;
    }
    break;
  case 0x34: // QueueNumMax
    switch (dev->QueueSel) {
    case 0:
      ret = 64;
      break;
    case 1:
      ret = 64;
      break;
    }
    break;
  case 0x44:
    ret = dev->queues[dev->QueueSel].QueueReady;
    break;
  case 0x70:
    ret = dev->Status;
    break;
  case 0xfc:
    // if this changes, there was a race condition between the host and guest, while accessing stuff at 0x100+
    // the guest will detect 0xfc changing, know it lost the race, and repeat the process
    ret = dev->ConfigGeneration;
    break;
  case 0x102:
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
  }
  if ( (offset >= 0x100) && (offset < (0x100 + 128))) {
    int newoff = offset - 0x100;
    ret = dev->type->config_load(dev, newoff);
  }
  printf("virtio%d_load(0x%x) == 0x%x\n", dev->index, offset, ret);
  return ret;
}
