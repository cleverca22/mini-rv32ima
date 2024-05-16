#include <stdio.h>
#include "virtio.h"
#include "plic.h"

static uint32_t virtio_snd_config_load(struct virtio_device *dev, uint32_t offset) {
  uint32_t ret = 0;
  printf("virtio_snd_config_load(%p, %d) == 0x%x\n", dev, offset, ret);
  return ret;
}

static void virtio_snd_config_store(struct virtio_device *dev, uint32_t offset, uint32_t val) {
  printf("virtio_snd_config_store(%p, %d, %d)\n", dev, offset, val);
}

static void virtio_snd_process_command(struct virtio_device *dev, struct virtio_desc_internal *chain, int chain_length, int queue, uint16_t start_idx) {
  printf("virtio_snd_process_command(%p, %p, %d, %d, %d)\n", dev, chain, chain_length, queue, start_idx);
}

static const virtio_device_type virtio_snd_type = {
  .device_type = 25,
  .queue_count = 4,
  .config_load = virtio_snd_config_load,
  .config_store = virtio_snd_config_store,
  .process_command = virtio_snd_process_command,
};

struct virtio_device *virtio_snd_create(void *ram_image, uint32_t base) {
  return virtio_create(ram_image, &virtio_snd_type, base, 0x200, get_next_irq());
}
