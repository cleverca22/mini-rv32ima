#include <assert.h>
#include <linux/input-event-codes.h>
#include <linux/virtio_input.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <X11/keysymdef.h>
#include "virtio.h"
#include "x11-key-lut.h"
#include "plic.h"
#include "rawdraw_sf.h"

typedef struct {
  struct input_queue *virtio_input_queue_head;
  int virtio_queue_size;
} virtio_input_instance;

static struct virtio_device *keyb, *mouse;

struct input_queue {
  uint8_t *dest;
  struct virtio_device *dev;
  struct virtio_desc_internal *chain;
  int queue;
  uint16_t start_idx;
  struct input_queue *next;
};

static bool relative_mode = false;
static int width = 640;
static int height = 480;

static uint8_t abs_axis_bitmap[1] = { 1<<ABS_X | 1<<ABS_Y };
static uint8_t rel_axis_bitmap[1] = { 1<<REL_X | 1<<REL_Y };

static const struct virtio_input_absinfo abs_x = {
  .min = 0,
  .max = 640,
};
static const struct virtio_input_absinfo abs_y = {
  .min = 0,
  .max = 480,
};

uint32_t virtio_input_config_load(struct virtio_device *dev, uint32_t offset) {
  //printf("input cfg ld 0x%x\n", offset);
  uint32_t ret = 0;

  switch (offset) {
  case 2: // size
    switch (dev->config_select) {
    case 1: // id name
      ret = strlen("full-rv32ima");
      break;
    case 0x11:
      // config_subsel is a type like EV_KEY
      // value returned at 0x108, should be a bitmap, of every code (KEY_C or REL_X) the device supports
      switch (dev->config_subsel) {
      case EV_KEY:
        ret = sizeof(valid_keys_bitmap); // device can send keys, and the bitmap of supported keys is $ret bytes
        break;
      case EV_ABS:
        ret = sizeof(abs_axis_bitmap);
        break;
      case EV_REL:
        ret = sizeof(rel_axis_bitmap);
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
        const char *str = "full-rv32ima";
        ret = str[offset - 8];
        break;
      case 0x11: // bitmap of key codes device supports
        switch (dev->config_subsel) {
        case EV_KEY:
          if ((offset-8) < sizeof(valid_keys_bitmap)) {
            ret = valid_keys_bitmap[offset-8];
          }
          break;
        case EV_ABS:
          if ((offset-8) < sizeof(abs_axis_bitmap)) {
            ret = abs_axis_bitmap[offset-8];
          }
          break;
        case EV_REL:
          if ((offset-8) < sizeof(rel_axis_bitmap)) {
            ret = rel_axis_bitmap[offset-8];
          }
          break;
        }
        break;
      case 0x12:
        switch (dev->config_subsel) {
        case ABS_X:
          if ((offset-8) < sizeof(struct virtio_input_absinfo)) {
            uint8_t *addr = (uint8_t*)&abs_x;
            ret = addr[offset-8];
          }
          break;
        case ABS_Y:
          if ((offset-8) < sizeof(struct virtio_input_absinfo)) {
            uint8_t *addr = (uint8_t*)&abs_y;
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
  virtio_input_instance *ctx = dev->type_context;

  struct input_queue *node = malloc(sizeof(struct input_queue));
  node->dest = chain[0].message;
  node->dev = dev;
  node->chain = chain;
  node->queue = queue;
  node->start_idx = start_idx;
  node->next = NULL;

  if (ctx->virtio_input_queue_head) {
    struct input_queue *tail = ctx->virtio_input_queue_head;
    //printf("initial tail %d -> %p", ctx->virtio_queue_size, tail);
    int l = 0;
    while (tail->next) {
      tail = tail->next;
      l++;
      //printf(" -> %p", tail);
    }
    //printf(" -> end %d\n", l);
    tail->next = node;
  } else {
    ctx->virtio_input_queue_head = node;
  }
  ctx->virtio_queue_size++;
}

void send_event(virtio_input_instance *ctx, uint16_t type, uint16_t code, uint32_t value) {
  //printf("send_event %d\n", virtio_queue_size);
  struct input_queue *node = ctx->virtio_input_queue_head;
  if (!node) {
    char *type_str = NULL;
    switch (type) {
    case EV_REL:
      type_str = "EV_REL";
      break;
    case EV_ABS:
      type_str = "EV_ABS";
      break;
    case EV_KEY:
      type_str = "EV_KEY";
      break;
    case EV_SYN:
      type_str = "EV_SYN";
      break;
    }
    printf("%s dropped, no buffer\n", type_str);
    return;
  }
  struct virtio_input_event *evt = (struct virtio_input_event*)node->dest;
  evt->type = type;
  evt->code = code;
  evt->value = value;
  virtio_flag_completion(node->dev, node->chain, node->queue, node->start_idx, 8);
  ctx->virtio_input_queue_head = node->next;
  memset(node, 0xaa, sizeof(*node));
  free(node);
  ctx->virtio_queue_size--;
}

#ifndef CNFGHTTP
static void recenter(void) {
  const int hw = width/2;
  const int hh = height/2;
  CNFGSetMousePosition(hw, hh);
}
#endif

#define VIRTIO_WEAK_HACK
// WEAK only works at link time
// if you cat *.c > foo.c, then gcc complains about the duplicates
// this flag tells the dups to go away

void HandleKey( int keycode, int bDown ) {
  assert(keyb);
  virtio_input_instance *ctx = keyb->type_context;
  // when rawdraw is using X11, keycode comes from X11/keysymdef.h, codes like XK_BackSpace
  // XK_A/0x41/65

  // when rawdraw is using http, keycode is from KeyboardEvent.keyCode
  // A is 0x41/65, when both uppercase and lowercase
#ifdef WITH_X11
  if (keycode == CNFG_X11_EXPOSE) {
    return;
  }
#endif
  //printf("virtio input HandleKey: %d/0x%x %d\n", keycode, keycode, bDown);
  int linux_code = translate_keycode(keycode);
  if (linux_code == 0) {
    printf("%d 0x%x not mapped\n", keycode, keycode);
    return;
  }

#ifndef CNFGHTTP
  if (linux_code == KEY_SCROLLLOCK) {
    // toggle relative vs abs mode
    if (bDown) {
      relative_mode = !relative_mode;
      printf("grabbing %d\n", relative_mode);
      CNFGConfineMouse(relative_mode);
      recenter();
    }
  } else
#endif
  {
    send_event(ctx, EV_KEY, linux_code, bDown);
    send_event(ctx, EV_SYN, 0, 0);
  }
}

void HandleMotion( int x, int y, int mask ) {
  if (mouse == NULL) {
    puts("no mouse ptr");
    return;
  }
  virtio_input_instance *ctx = mouse->type_context;
  if (relative_mode) {
    const int hw = width/2;
    const int hh = height/2;
    const int delta_x = x - hw;
    const int delta_y = y - hh;
    if ((delta_x != 0) || (delta_y != 0)) {
      //printf("HandleMotion(%3d, %3d)\n", delta_x, delta_y);
      send_event(ctx, EV_REL, REL_X, delta_x);
      send_event(ctx, EV_REL, REL_Y, delta_y);
      recenter();
    }
  } else {
    //printf("HandleMotion(%3d, %3d)\n", x, y);
    send_event(ctx, EV_ABS, ABS_X, x);
    send_event(ctx, EV_ABS, ABS_Y, y);
  }
  send_event(ctx, EV_SYN, 0, 0);
}

void HandleButton( int x, int y, int button, int bDown ) {
  virtio_input_instance *ctx = mouse->type_context;
  switch (button) {
  case 1:
    send_event(ctx, EV_KEY, BTN_LEFT, bDown);
    break;
  case 2:
    send_event(ctx, EV_KEY, BTN_MIDDLE, bDown);
    break;
  case 3:
    send_event(ctx, EV_KEY, BTN_RIGHT, bDown);
    break;
  }
  send_event(ctx, EV_SYN, 0, 0);
}

static const virtio_device_type virtio_input_type = {
  .device_type = 18,
  .queue_count = 2,
  .config_load = virtio_input_config_load,
  .config_store = virtio_input_config_store,
  .process_command = virtio_input_process_command,
};

struct virtio_device *virtio_input_create(void *ram_image, uint32_t base, bool mouse_mode) {
  struct virtio_device *dev = virtio_create(ram_image, &virtio_input_type, base, 0x200, get_next_irq());
  virtio_input_instance *ctx = malloc(sizeof(virtio_input_instance));
  bzero(ctx, sizeof(virtio_input_instance));
  dev->type_context = ctx;

  if (mouse_mode) {
    mouse = dev;
  } else {
    keyb = dev;
  }
  return dev;
}
