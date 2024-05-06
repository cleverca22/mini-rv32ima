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
static int fd = -1;
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
};

void I_InitInput(void) {
  fd = open("/dev/input/event0", O_RDWR | O_NONBLOCK);
  if (fd < 0) {
    perror("cant open event0");
  }
  assert(fd >= 0);

  int ret = ioctl(fd, EVIOCGRAB, (void*)1);
  assert(ret == 0);
}

void I_GetEvent(void) {
  struct input_event ev[64];
  int rd = read(fd, ev, sizeof(ev));
  for (int j=0; j < rd / ((signed int)sizeof(struct input_event)); j++) {
    event_t event;
    if (ev[j].type == EV_KEY) {
      if (ev[j].code == KEY_LEFTCTRL) {
        shiftdown = (ev[j].value > 0);
      } else if ((ev[j].code < sizeof(ev_to_doom)) && (ev_to_doom[ev[j].code])) {
        if (ev[j].value) event.type = ev_keydown;
        else event.type = ev_keyup;

        event.data1 = ev_to_doom[ev[j].code];
        if (shiftdown) {
        } else {
          event.data2 = event.data1;
        }
        D_PostEvent(&event);
      }
    } else if (ev[j].type == EV_SYN) {
    } else if (ev[j].type == EV_ABS) { // mouse
    } else {
      printf("sec: %ld.%06ld type: %d code: %d value: %d\n", ev[j].input_event_sec, ev[j].input_event_usec, ev[j].type, ev[j].code, ev[j].value);
    }
  }
}
