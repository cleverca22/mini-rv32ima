#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "d_event.h"
#include "doomkeys.h"

int vanilla_keyboard_mapping = 1;
static int fd0 = -1, fd1 = -1;
static int shiftdown = 0;

static const char ev_to_doom[] = {
  [KEY_0] = '0',
  [KEY_1] = '1',
  [KEY_2] = '2',
  [KEY_3] = '3',
  [KEY_4] = '4',
  [KEY_5] = '5',
  [KEY_6] = '6',
  [KEY_7] = '7',
  [KEY_8] = '8',
  [KEY_9] = '9',
  [KEY_COMPOSE] = KEY_FIRE,
  [KEY_C] = 'c',
  [KEY_DOWN] = KEY_DOWNARROW,
  [KEY_ENTER] = KEY_ENTER,
  [KEY_ESC] = KEY_ESCAPE,
  [KEY_LEFT] = KEY_LEFTARROW,
  [KEY_RIGHTCTRL] = KEY_FIRE,
  [KEY_RIGHTSHIFT] = KEY_RSHIFT,
  [KEY_RIGHT] = KEY_RIGHTARROW,
  [KEY_UP] = KEY_UPARROW,
  [KEY_Y] = 'y',
  [KEY_SPACE] = KEY_USE,
};

void I_InitInput(void) {
  fd0 = open("/dev/input/event0", O_RDWR | O_NONBLOCK);
  if (fd0 < 0) {
    perror("cant open event0");
  }
  assert(fd0 >= 0);

  int ret = ioctl(fd0, EVIOCGRAB, (void*)1);
  assert(ret == 0);

  fd1 = open("/dev/input/event1", O_RDWR | O_NONBLOCK);
  if (fd1 < 0) {
    perror("cant open event1");
  }
  assert(fd1 >= 0);

  ret = ioctl(fd1, EVIOCGRAB, (void*)1);
  assert(ret == 0);
}

int delta_x = 0;
int delta_y = 0;
uint8_t mouse_bits = 0;
uint8_t old_mouse_bits = 0;

void check_for_events(int fd) {
  struct input_event ev[32];
  int rd = read(fd, ev, sizeof(ev));
  for (int j=0; j < rd / ((signed int)sizeof(struct input_event)); j++) {
    event_t event;
    if (ev[j].type == EV_KEY) {
      if (ev[j].code == BTN_LEFT) {
        if (ev[j].value == 0) { // up
          mouse_bits &= ~1;
        } else { // down
          mouse_bits |= 1;
        }
      } if (ev[j].code == KEY_LEFTCTRL) {
        shiftdown = (ev[j].value > 0);
      } else if ((ev[j].code < sizeof(ev_to_doom)) && (ev_to_doom[ev[j].code])) {
        switch (ev[j].value) {
        case 0: // release
          event.type = ev_keyup;
          event.data1 = ev_to_doom[ev[j].code];
          event.data2 = event.data1;
          D_PostEvent(&event);
          break;
        case 1: // initial down
          event.type = ev_keydown;
          event.data1 = ev_to_doom[ev[j].code];
          event.data2 = event.data1;
          D_PostEvent(&event);
          break;
        case 2: // repeat
          //printf("r");
          break;
        }

      }
    } else if (ev[j].type == EV_SYN) {
      if ((delta_x != 0) || (delta_y != 0) || (mouse_bits != old_mouse_bits)) {
        event.type = ev_mouse;
        event.data1 = mouse_bits;
        event.data2 = delta_x * 4;
        event.data3 = delta_y;
        event.data3 = 0; // no y movement!
        D_PostEvent(&event);
        delta_x = 0;
        delta_y = 0;
        old_mouse_bits = mouse_bits;
      }
    } else if (ev[j].type == EV_ABS) { // mouse
    } else if (ev[j].type == EV_REL) { // mouse
      if (ev[j].code == REL_X) delta_x = ev[j].value;
      else if (ev[j].code == REL_Y) delta_y = ev[j].value;
    } else {
      printf("sec: %ld.%06ld type: %d code: %d value: %d\n", ev[j].input_event_sec, ev[j].input_event_usec, ev[j].type, ev[j].code, ev[j].value);
    }
  }
}

void I_GetEvent(void) {
  check_for_events(fd0);
  check_for_events(fd1);
}
