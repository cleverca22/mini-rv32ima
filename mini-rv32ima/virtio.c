#include <strings.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <libfdt.h>

#include "virtio.h"
#include "plic.h"

static struct virtio_device *virtio_devices[64];
static int virtio_count = 0;

struct virtio_device *virtio_create(void *ram_image, const virtio_device_type *type, uint32_t base, uint32_t size, int irq) {
  int queues = type->queue_count;
  struct virtio_device *dev = malloc(sizeof(struct virtio_device));
  bzero(dev, sizeof(struct virtio_device));
  dev->index = virtio_count;
  dev->type = type;
  dev->reg_base = base;
  dev->reg_size = size;
  dev->ram_image = ram_image;
  dev->irq = irq;
  dev->ConfigGeneration = 0;
  dev->queues = malloc(sizeof(virtio_queue) * queues);
  dev->queue_count = queues;
  for (int i=0; i<queues; i++) {
    dev->queues[i].QueueReady = 0;
    dev->queues[i].write_ptr = 0;
  }
  virtio_devices[virtio_count] = dev;
  virtio_count++;
  return dev;
}

void hexdump_ram(void *ram_image, uint32_t addr, uint32_t len) {
  for (int i=0; i<len; i++) {
    uint32_t addy = addr + i;
    if ((i % 16) == 0) printf("0x%x: ", addy);

    uint8_t t = *((uint8_t*)(ram_image + (addy - 0x80000000)));
    printf("%02x ", t);
    if ((i % 16) == 15) printf("\n");
  }
  printf("\n");
}

static void virtio_dump_desc(struct virtio_device *dev, const virtio_desc *desc) {
  printf("\taddr: 0x%lx\n", desc->addr);
  printf("\tlen: %d\n", desc->len);
  printf("\tflags: 0x%x\n", desc->flags);
  if (desc->flags & 1) puts("\t\tVIRTQ_DESC_F_NEXT");
  if (desc->flags & 2) puts("\t\tVIRTQ_DESC_F_WRITE"); // host->guest
  if (desc->flags & 4) puts("\t\tVIRTQ_DESC_F_INDIRECT");
  printf("\tnext: %d\n", desc->next);
  if (desc->len != 4096) {
    //hexdump_ram(dev->ram_image, desc->addr, desc->len);
  } else puts("buffer omitted");
}

static void virtio_dump_available(struct virtio_device *dev, int queue) {
  virtio_available_ring *ring = cast_guest_ptr(dev->ram_image, dev->queues[queue].QueueDriverLow);
  virtio_desc *table = cast_guest_ptr(dev->ram_image, dev->queues[queue].QueueDescLow);

  puts("avail ring:");
  hexdump_ram(dev->ram_image, dev->queues[queue].QueueDriverLow, 32);

  printf("dumping available queue %d\n", queue);
  printf("base addr: 0x%x\n", dev->queues[queue].QueueDriverLow);
  printf("flags: 0x%x\n", ring->flags);
  printf("idx: %d\n", ring->idx);

  hexdump_ram(dev->ram_image, dev->queues[queue].QueueDescLow, 64);

  for (int n=dev->queues[queue].read_ptr; ; n++) {
    if (n == ring->idx) {
      puts("hit write ptr");
      break;
    }
    uint16_t n_capped = n % dev->queues[queue].QueueNum;
    uint16_t idx = ring->ring[n_capped];
    uint16_t idx_capped = idx % dev->queues[queue].QueueNum;
    printf("slot[%d(%d)]: %d(%d)\n", n, n_capped, idx, idx_capped);
    if (idx_capped < 64) {
      virtio_desc *desc = &table[idx_capped];
again:
      virtio_dump_desc(dev, desc);
      if (desc->flags & 1) {
        idx = desc->next;
        desc = &table[idx];
        goto again;
      }
    }
  }
  // VIRTIO_F_EVENT_IDX(29)
  printf("used_event: %d\n", ring->ring[64]);
}

void virtio_process_rings(struct virtio_device *dev) {
  int total_chains = 0;
  for (int queue=0; queue < dev->queue_count; queue++) {
    virtio_desc *table = cast_guest_ptr(dev->ram_image, dev->queues[queue].QueueDescLow);
    virtio_available_ring *ring = cast_guest_ptr(dev->ram_image, dev->queues[queue].QueueDriverLow);
    //printf(RED"read ptr %d\n"DEFAULT, dev->queues[queue].read_ptr);
    for (uint16_t n=dev->queues[queue].read_ptr; ; n++) {
      if (n == ring->idx) {
        dev->queues[queue].read_ptr = n;
        //puts("hit write ptr");
        break;
      }
      uint16_t n_capped = n % dev->queues[queue].QueueNum;
      int descriptor_chain_length = 0;
      uint16_t idx = ring->ring[n_capped];
      uint16_t start_idx = idx;
      //printf("slot[%d(%d)]: %d\n", n, n_capped, idx);
again:
      virtio_desc *desc = &table[idx];
      descriptor_chain_length++;
      if (desc->flags & 1) {
        idx = desc->next;
        goto again;
      }
      //printf("total length: %d\n", descriptor_chain_length);
      struct virtio_desc_internal *chain = malloc(sizeof(struct virtio_desc_internal) * descriptor_chain_length);

      int slot = 0;
      idx = start_idx;
again2:
      desc = &table[idx];
      chain[slot].message = cast_guest_ptr(dev->ram_image, desc->addr);
      chain[slot].message_len = desc->len;
      chain[slot].flags = desc->flags;
      slot++;
      if (desc->flags & 1) {
        idx = desc->next;
        goto again2;
      }
      //printf(RED"command at idx %d(%d)->%d with %d buffers\n"DEFAULT, n, n_capped, start_idx, descriptor_chain_length);
      assert(dev->type->process_command);
      dev->type->process_command(dev, chain, descriptor_chain_length, queue, start_idx);
      total_chains++;
    }
  }
  //printf(RED"%d total IO in this kick\n"DEFAULT, total_chains);
}

void virtio_flag_completion(struct virtio_device *dev, struct virtio_desc_internal *chain, int queue, uint16_t start_idx, uint32_t written) {
  struct virtio_used_ring *ring = cast_guest_ptr(dev->ram_image, dev->queues[queue].QueueDeviceLow);
  // TODO, protect with a mutex
  int index = dev->queues[queue].write_ptr % dev->queues[queue].QueueNum;

  ring->ring[index].id = start_idx;
  ring->ring[index].written = written;

  dev->queues[queue].write_ptr++;
  ring->idx = dev->queues[queue].write_ptr;
  dev->InterruptStatus |= 1;
  plic_raise_irq(dev->irq);
  free(chain);
  //hexdump_ram(dev->ram_image, dev->queues[queue].QueueDeviceLow, 32);
  //printf(RED"command at idx %d completed into %d(%d), %d written\n"DEFAULT, start_idx, dev->queues[queue].write_ptr - 1, index, written);
}

void virtio_config_changed(struct virtio_device *dev) {
  dev->ConfigGeneration++;
  dev->InterruptStatus |= 2;
  plic_raise_irq(dev->irq);
}

static void virtio_dump_rings(struct virtio_device *dev) {
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

void virtio_mmio_store(void *state, uint32_t offset, uint32_t val) {
  struct virtio_device *dev = state;
  //printf("virtio%d_store(0x%x, 0x%x)\n", dev->index, offset, val);
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
    printf("virtio%d QueueSel = %d\n", dev->index, val);
    break;
  case 0x38:
    dev->queues[dev->QueueSel].QueueNum = val;
    printf("virtio%d QueueNum[%d] = %d\n", dev->index, dev->QueueSel, val);
    break;
  case 0x44:
    dev->queues[dev->QueueSel].QueueReady = val;
    if (val == 1) {
      printf("virtio%d Queue[%d] has become ready\n", dev->index, dev->QueueSel);
    }
    break;
  case 0x50: // QueueNotify
    //printf("QueueNotify %d\n", val);
    //virtio_dump_rings(dev);
    virtio_process_rings(dev);
    break;
  case 0x64: // InterruptACK
    dev->InterruptStatus &= ~val;
    if (dev->InterruptStatus == 0) {
      plic_clear_irq(dev->irq);
    }
    break;
  case 0x70:
    dev->Status = val;
    if (val == 0) {
      printf("virtio%d reset\n", dev->index);
      dev->InterruptStatus = 0;
      for (int queue=0; queue < dev->queue_count; queue++) {
        dev->queues[queue].read_ptr = 0;
      }
    } else {
      //if (val & 1) puts("VIRTIO_CONFIG_S_ACKNOWLEDGE");
      //if (val & 2) puts("VIRTIO_CONFIG_S_DRIVER (driver found for dev)");
      //if (val & 4) puts("VIRTIO_CONFIG_S_DRIVER_OK");
      //if (val & 8) puts("VIRTIO_CONFIG_S_FEATURES_OK");
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
  }
  if ( (offset >= 0x100) && (offset < (0x100 + 128))) {
    int newoff = offset - 0x100;
    dev->type->config_store(dev, newoff, val);
  }
}

uint32_t virtio_mmio_load(void *state, uint32_t offset) {
  struct virtio_device *dev = state;
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
    if (dev->type->feature_array) {
      if (dev->DeviceFeaturesSel < dev->type->feature_array_length) {
        ret = dev->type->feature_array[dev->DeviceFeaturesSel];
      } else {
        ret = 0;
      }
    } else {
      switch (dev->DeviceFeaturesSel) {
      case 1:
        ret = 1;
        break;
      }
    }
    printf("virtio%d DeviceFeatures[%d] == 0x%x\n", dev->index, dev->DeviceFeaturesSel, ret);
    break;
  case 0x34: // QueueNumMax
    if (dev->type->queue_sizes) {
      ret = dev->type->queue_sizes[dev->QueueSel];
    } else {
      printf("warning, virtio%d lacks a queue_sizes entry\n", dev->index);
      // TODO, delegate this to the type
      switch (dev->QueueSel) {
      case 0:
        ret = 4096;
        break;
      case 1:
        ret = 64;
        break;
      case 2:
        ret = 4096;
        break;
      case 3:
        ret = 4096;
        break;
      }
    }
    printf("virtio%d QueueNumMax[%d] = %d\n", dev->index, dev->QueueSel, ret);
    break;
  case 0x44:
    ret = dev->queues[dev->QueueSel].QueueReady;
    break;
  case 0x60:
    ret = dev->InterruptStatus;
    break;
  case 0x70:
    ret = dev->Status;
    break;
  case 0xfc:
    // if this changes, there was a race condition between the host and guest, while accessing stuff at 0x100+
    // the guest will detect 0xfc changing, know it lost the race, and repeat the process
    ret = dev->ConfigGeneration;
    break;
  }
  if ( (offset >= 0x100) && (offset < (0x100 + 128))) {
    int newoff = offset - 0x100;
    ret = dev->type->config_load(dev, newoff);
  }
  //printf("virtio%d_load(0x%x) == 0x%x\n", dev->index, offset, ret);
  return ret;
}

void virtio_add_dtb(struct virtio_device *dev, void *v_fdt) {
  int soc = fdt_path_offset(v_fdt, "/soc");
  char buffer[32];
  snprintf(buffer, 32, "virtio@%x", dev->reg_base);
  int virtio = fdt_add_subnode(v_fdt, soc, buffer);
  fdt_appendprop_addrrange(v_fdt, soc, virtio, "reg", dev->reg_base, dev->reg_size);
  fdt_setprop_u32(v_fdt, virtio, "interrupts", dev->irq);
  fdt_setprop_string(v_fdt, virtio, "compatible", "virtio,mmio");
}
