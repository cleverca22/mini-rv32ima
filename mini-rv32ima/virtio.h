#pragma once

#include <stdint.h>
#include <stdbool.h>

#define CSI "\x1b["
#define RED     CSI"31m"
#define GREEN   CSI"32m"
#define DEFAULT CSI"39m"

typedef struct {
  uint16_t flags;
  uint16_t idx;
  uint16_t ring[];
} virtio_available_ring;

struct virtio_used_element {
  uint32_t id;
  uint32_t written;
};

struct virtio_used_ring {
  uint16_t flags;
  uint16_t idx;
  struct virtio_used_element ring[];
};

typedef struct {
  uint64_t addr;
  uint32_t len;
  uint16_t flags;
  uint16_t next;
} virtio_desc;

struct virtio_desc_internal {
  void *message;
  uint32_t message_len;
  uint16_t flags;
};

typedef struct {
  // descriptor ring
  // size, 16 * items
  // alignment, 16
  // written by guest
  uint32_t QueueDescLow;
  // available ring
  // size, 6 + (2 * items)
  // alignment, 2
  // written by guest, contains index of items in descriptor ring
  uint32_t QueueDriverLow;
  // used ring, contains index of descriptor ring items, upon completion
  // size, 6 + (8 * items)
  // alignment, 4
  // written by host
  uint32_t QueueDeviceLow;
  uint32_t QueueNum;        // 0x38
  uint32_t QueueReady;      // 0x44
  uint16_t read_ptr;        // the next element to read from the available ring
  uint16_t write_ptr;       // the next element the host writes to, in the used ring
} virtio_queue;

// x11(via rawdraw) headers leak a Status->int define
#undef Status

struct virtio_device;

typedef struct {
  uint32_t device_type;
  int queue_count;
  uint32_t (*config_load)(struct virtio_device *dev, uint32_t offset);
  void (*config_store)(struct virtio_device *dev, uint32_t offset, uint32_t val);
  void (*process_command)(struct virtio_device *dev, struct virtio_desc_internal *chain, int chain_length, int queue, uint16_t start_idx);
} virtio_device_type;

struct virtio_device {
  int index;
  const virtio_device_type *type;
  uint32_t reg_base;
  uint32_t reg_size;
  void *ram_image;
  int irq;
  uint32_t DeviceID;          // 0x08
  uint32_t DeviceFeaturesSel; // 0x14
  uint32_t DriverFeaturesSel; // 0x24
  uint32_t QueueSel;          // 0x30
  uint32_t InterruptStatus;   // 0x60
  uint32_t Status;            // 0x70
  uint32_t ConfigGeneration;  // 0xfc
  virtio_queue *queues;
  int queue_count;
  int config_select, config_subsel;
};

struct virtio_device *virtio_create(void *ram_image, const virtio_device_type *type, uint32_t base, uint32_t size, int irq);
void virtio_raise_irq();
void virtio_maybe_clear_irq();
void virtio_dump_all();
bool virtio_store(uint32_t addr, uint32_t val);
uint32_t virtio_load(uint32_t addr);
void *cast_guest_ptr(void *image, uint32_t addr);
void virtio_flag_completion(struct virtio_device *dev, int queue, uint16_t start_idx, uint32_t written);
void virtio_config_changed(struct virtio_device *dev);
void virtio_input_init();

extern const virtio_device_type virtio_blk_type;
extern const virtio_device_type virtio_input_type;
