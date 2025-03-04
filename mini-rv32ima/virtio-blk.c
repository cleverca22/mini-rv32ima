#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "virtio.h"
#include "plic.h"

// TODO, just #include them from linux headers?
struct virtio_blk_req {
  uint32_t type;
  uint32_t reserved;
  uint64_t sector;
};

static int fh = 0;
static uint64_t sector_count = 0;

static void openfile(struct virtio_device *dev) {
  if (fh) {
  } else {
    fh = open("disk.img", O_RDWR | O_CLOEXEC);
    assert(fh > 0);
  }
  uint64_t size = lseek(fh, 0, SEEK_END);
  uint64_t sectors = size / 512;
  if (sectors != sector_count) {
    sector_count = sectors;
    virtio_config_changed(dev, false);
  }
}

static uint32_t virtio_blk_config_load(struct virtio_device *dev, uint32_t offset) {
  openfile(dev);
  uint32_t ret = 0;
  switch (offset) {
  case 0: // lower 32bits of size
    ret = sector_count & 0xffffffff;
    break;
  case 4: // upper 32bits of size
    ret = sector_count >> 32;
    printf("%ld %d\n", sector_count, ret);
    break;
  }
  printf("virtio_blk_config_load(%p, %d) == 0x%x\n", dev, offset, ret);
  return ret;
}

static void virtio_blk_config_store(struct virtio_device *dev, uint32_t offset, uint32_t val) {
}

static void virtio_blk_process_command(struct virtio_device *dev, virtio_chain *chain, int queue, uint16_t start_idx) {
  openfile(dev);
  assert(chain->chain_length == 3);
  assert(chain->chain[0].message_len == 16);
  assert((chain->chain[0].flags & 2) == 0);
  const struct virtio_blk_req *req = chain->chain[0].message;
  void *buffer = chain->chain[1].message;
  assert(chain->chain[2].message_len == 1);
  assert(chain->chain[2].flags & 2);
  uint8_t *status = chain->chain[2].message;
  uint32_t written = 0;
  int ret;

  printf(GREEN"type: %d, sector: %ld bytes: %d\n"DEFAULT, req->type, req->sector, chain->chain[1].message_len);
  switch (req->type) {
  case 0: // read
    assert(chain->chain[1].flags & 2); // buffer must be write type
    ret = pread(fh, buffer, chain->chain[1].message_len, req->sector * 512);
    if (ret == chain->chain[1].message_len) {
      *status = 0;
      written = chain->chain[1].message_len + 1;
    } else {
      written = chain->chain[1].message_len + 1;
      *status = 1;
    }
    break;
  case 1: // write
    assert((chain->chain[1].flags & 2) == 0); // buffer must be read type
    ret = pwrite(fh, buffer, chain->chain[1].message_len, req->sector * 512);
    if (ret == chain->chain[1].message_len) {
      *status = 0;
      written = chain->chain[1].message_len + 1;
    } else {
      written = chain->chain[1].message_len + 1;
      *status = 1;
    }
    break;
  default:
    *status = 2;
    written = 1;
    break;
  }
  chain->bytes_written = written;
  virtio_flag_completion(dev, chain, queue, start_idx, false);
}

static const virtio_device_type virtio_blk_type = {
  .device_type = 2,
  .queue_count = 1,
  .config_load = virtio_blk_config_load,
  .config_store = virtio_blk_config_store,
  .process_command = virtio_blk_process_command,
};

struct virtio_device *virtio_blk_create(void *ram_image, uint32_t base) {
  return virtio_create(ram_image, &virtio_blk_type, base, 0x200, get_next_irq());
}
