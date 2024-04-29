#include <assert.h>
#include <linux/input.h>
#include <linux/virtio_input.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "virtio.h"

struct input_queue {
  uint8_t *dest;
  struct virtio_device *dev;
  int queue;
  uint16_t start_idx;
  struct input_queue *next;
};

struct input_queue *virtio_input_queue_head = NULL;

static const uint16_t key_translate[65536] = {
  [32] = KEY_SPACE,
  //[39] = '
  [44] = KEY_COMMA,
  [45] = KEY_MINUS,
  [46] = KEY_DOT,
  [47] = KEY_SLASH,
  [48] = KEY_0,
  [49] = KEY_1,
  [50] = KEY_2,
  [51] = KEY_3,
  [52] = KEY_4,
  [57] = KEY_9,
  [59] = KEY_SEMICOLON,
  [61] = KEY_EQUAL,
  //[91] = [
  [92] = KEY_BACKSLASH,
  //[93] = ]
  [97] = KEY_A,
  [98] = KEY_B,
  [99] = KEY_C,
  [100] = KEY_D,
  [101] = KEY_E,
  [102] = KEY_F,
  [103] = KEY_G,
  [104] = KEY_H,
  [105] = KEY_I,
  [106] = KEY_J,
  [107] = KEY_K,
  [108] = KEY_L,
  [109] = KEY_M,
  [110] = KEY_N,
  [111] = KEY_O,
  [112] = KEY_P,
  [113] = KEY_Q,
  [114] = KEY_R,
  [115] = KEY_S,
  [116] = KEY_T,
  [117] = KEY_U,
  [118] = KEY_V,
  [119] = KEY_W,
  [120] = KEY_X,
  [121] = KEY_Y,
  [122] = KEY_Z,
  [65293] = KEY_ENTER,
  [65505] = KEY_LEFTSHIFT,
  [65507] = KEY_LEFTCTRL,
};

static uint8_t key_bitmap[8];
static uint8_t axis_bitmap[1] = { 3 };
static const struct virtio_input_absinfo abs_x = {
  .min = 0,
  .max = 640,
};
static const struct virtio_input_absinfo abs_y = {
  .min = 0,
  .max = 480,
};

void virtio_input_init() {
  for (int i=0; i < (sizeof(key_translate)/sizeof(key_translate[0])); i++) {
    if (key_translate[i]) {
      unsigned int code = key_translate[i];
      unsigned int byte = code / 8;
      unsigned int bit = code % 8;
      printf("%u %u %u %u\n", i, code, byte, bit);
      assert(byte < sizeof(key_bitmap));
      key_bitmap[byte] |= 1<<bit;
    }
  }
  for (int i=0; i<sizeof(key_bitmap); i++) {
    printf("%d %02x\n", i, key_bitmap[i]);
  }
}

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
      switch (dev->config_subsel) {
      case EV_KEY:
        ret = sizeof(key_bitmap); // device can send keys, and the bitmap of supported keys is 4 bytes
        break;
      case EV_ABS:
        ret = sizeof(axis_bitmap);
        break;
      }
      break;
    case 0x12:
      // absolute pointer space
      // linux will only read it, if the above bitfield reports a EV_ABS type
      // it will then check for each bit (such as 1<<ABS_X), and query the range for that axis
      switch (dev->config_subsel) {
      case ABS_X:
      case ABS_Y:
        ret = sizeof(struct virtio_input_absinfo);
        break;
      }
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
      case 0x11: // bitmap of key codes device supports
        switch (dev->config_subsel) {
        case EV_KEY:
          if ((offset-8) < sizeof(key_bitmap)) {
            ret = key_bitmap[offset-8];
          }
          break;
        case EV_ABS:
          if ((offset-8) < sizeof(axis_bitmap)) {
            ret = axis_bitmap[offset-8];
          }
          break;
        }
        break;
      case 0x12:
        switch (dev->config_subsel) {
        case ABS_X:
          if ((offset-8) < sizeof(struct virtio_input_absinfo)) {
            uint8_t *addr = &abs_x;
            ret = addr[offset-8];
          }
          break;
        case ABS_Y:
          if ((offset-8) < sizeof(struct virtio_input_absinfo)) {
            uint8_t *addr = &abs_y;
            ret = addr[offset-8];
          }
          break;
        }
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

static void virtio_input_process_command(struct virtio_device *dev, struct virtio_desc_internal *chain, int chain_length, int queue, uint16_t start_idx) {
  assert(chain_length == 1);
  assert(chain[0].message_len == 8);
  assert(chain[0].flags == 2);
  assert(queue == 0);

  struct input_queue *node = malloc(sizeof(struct input_queue));
  node->dest = chain[0].message;
  node->dev = dev;
  node->queue = queue;
  node->start_idx = start_idx;
  node->next = NULL;

  if (virtio_input_queue_head) {
    struct input_queue *tail = virtio_input_queue_head;
    while (tail->next) {
      tail = tail->next;
    }
    tail->next = node;
  } else {
    virtio_input_queue_head = node;
  }
}

void send_event(uint16_t type, uint16_t code, uint32_t value) {
  struct input_queue *node = virtio_input_queue_head;
  if (!node) {
    puts("key dropped, no buffer");
    return;
  }
  struct virtio_input_event *evt = node->dest;
  evt->type = type;
  evt->code = code;
  evt->value = value;
  virtio_flag_completion(node->dev, node->queue, node->start_idx, 8);
  virtio_input_queue_head = node->next;
  free(node);
}

void HandleKey( int keycode, int bDown ) {
  printf("virtio input HandleKey: %d %d\n", keycode, bDown);
  int key2 = key_translate[keycode];
  if (key2 == 0) {
    printf("%d not mapped\n", keycode);
    return;
  }
  send_event(EV_KEY, key2, bDown);
  send_event(EV_SYN, 0, 0);
}

void HandleMotion( int x, int y, int mask ) {
  send_event(EV_ABS, ABS_X, x);
  send_event(EV_ABS, ABS_Y, y);
  send_event(EV_SYN, 0, 0);
}

const virtio_device_type virtio_input_type = {
  .device_type = 18,
  .queue_count = 2,
  .config_load = virtio_input_config_load,
  .config_store = virtio_input_config_store,
  .process_command = virtio_input_process_command,
};
