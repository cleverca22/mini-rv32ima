#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define CSI "\x1b["
#define RED     CSI"31m"
#define GREEN   CSI"32m"
#define DEFAULT CSI"39m"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

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
#define VIRTQ_DESC_F_NEXT       1
// write is host->guest
#define VIRTQ_DESC_F_WRITE      2
#define VIRTQ_DESC_F_INDIRECT   4
  uint16_t flags;
  uint16_t next;
} virtio_desc;

struct virtio_desc_internal {
  void *message;
  uint32_t message_len;
  uint16_t flags;
};

typedef struct {
  struct virtio_desc_internal *chain;
  int chain_length;
  int seek_chain;
  int seek_bytes;
  int readable_bytes;
  int writeable_bytes;
  int bytes_written;
} virtio_chain;

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
  const int *queue_sizes;
  const uint32_t *feature_array;
  uint32_t feature_array_length;
  uint32_t (*config_load)(struct virtio_device *dev, uint32_t offset);
  void (*config_store)(struct virtio_device *dev, uint32_t offset, uint32_t val);
  void (*process_command)(struct virtio_device *dev, virtio_chain *chain, int queue, uint16_t start_idx);
} virtio_device_type;

struct virtio_device {
  int index;
  const virtio_device_type *type;
  void *type_context;
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
void virtio_raise_irq(void);
void virtio_maybe_clear_irq(void);
void virtio_dump_all(void);

bool virtio_store(uint32_t addr, uint32_t val);
uint32_t virtio_load(uint32_t addr);

void virtio_mmio_store(void *dev, uint32_t offset, uint32_t val);
uint32_t virtio_mmio_load(void *dev, uint32_t offset);

void *cast_guest_ptr(void *image, uint32_t addr);
void virtio_flag_completion(struct virtio_device *dev, virtio_chain * chain, int queue, uint16_t start_idx, bool need_lock);
void virtio_config_changed(struct virtio_device *dev, bool need_lock);
struct virtio_device *virtio_input_create(void *ram_image, uint32_t base, bool mouse);
struct virtio_device *virtio_blk_create(void *ram_image, uint32_t base);
struct virtio_device *virtio_snd_create(void *ram_image, uint32_t base);
struct virtio_device *virtio_net_create(void *ram_image, uint32_t base);
void virtio_add_dtb(struct virtio_device*, void *v_fdt);
void hexdump_ram(void *ram_image, uint32_t addr, uint32_t len);
void virtio_net_teardown(void);

static inline void virtio_dump_chain(virtio_chain *chain) {
  for (int i=0; i<chain->chain_length; i++) {
    printf("link %d\n", i);
    printf("  host ptr: %p\n", chain->chain[i].message);
    printf("  len: %d\n", chain->chain[i].message_len);
    printf("  flags: 0x%x\n", chain->chain[i].flags);
  }
}

static inline void virtio_chain_zero(virtio_chain *chain, int bytes) {
  assert(chain->readable_bytes == 0);
  assert(bytes <= chain->writeable_bytes);

  chain->bytes_written += bytes;
  chain->writeable_bytes -= bytes;
  while (bytes) {
    int buf_size = chain->chain[chain->seek_chain].message_len;
    int buf_remain = buf_size - chain->seek_bytes;
    int len_min = MIN(bytes, buf_remain);
    //printf("zeroing %d bytes at %d+%d\n", len_min, chain->seek_chain, chain->seek_bytes);
    memset(chain->chain[chain->seek_chain].message + chain->seek_bytes, 0, len_min);
    chain->seek_bytes += len_min;
    bytes -= len_min;

    if (chain->seek_bytes == chain->chain[chain->seek_chain].message_len) {
      chain->seek_bytes = 0;
      chain->seek_chain++;
    }
  }
}

static inline void virtio_chain_write(virtio_chain *chain, uint8_t *buf, int bytes) {
  assert(chain->readable_bytes == 0);
  assert(bytes <= chain->writeable_bytes);

  chain->bytes_written += bytes;
  chain->writeable_bytes -= bytes;
  while (bytes) {
    int buf_size = chain->chain[chain->seek_chain].message_len;
    int buf_remain = buf_size - chain->seek_bytes;
    int len_min = MIN(bytes, buf_remain);
    memcpy(chain->chain[chain->seek_chain].message + chain->seek_bytes, buf, len_min);
    buf += len_min;
    chain->seek_bytes += len_min;
    bytes -= len_min;

    if (chain->seek_bytes == chain->chain[chain->seek_chain].message_len) {
      chain->seek_bytes = 0;
      chain->seek_chain++;
    }
  }
}

static inline void virtio_chain_read(virtio_chain *chain, uint8_t *buf, int bytes) {
  assert(bytes <= chain->readable_bytes);
}
