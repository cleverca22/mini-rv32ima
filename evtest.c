// #define __USE_TIME_BITS64
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
//#include <linux/input-compat.h>

struct input_event_compat {
  uint64_t time;
  uint16_t type;
  uint16_t code;
  uint32_t value;
};

char keys[27];

uint8_t mapping[] = {
  [KEY_A] = 0,
  [KEY_B] = 1,
  [KEY_C] = 2,
  [KEY_D] = 3,
  [KEY_E] = 4,
  [KEY_F] = 5,
  [KEY_G] = 6,
  [KEY_H] = 7,
  [KEY_I] = 8,
  [KEY_J] = 9,
  [KEY_K] = 10,
  [KEY_L] = 11,
  [KEY_M] = 12,
  [KEY_N] = 13,
  [KEY_O] = 14,
  [KEY_P] = 15,
  [KEY_Q] = 16,
  [KEY_R] = 17,
  [KEY_S] = 18,
  [KEY_T] = 19,
  [KEY_U] = 20,
  [KEY_V] = 21,
  [KEY_W] = 22,
  [KEY_X] = 23,
  [KEY_Y] = 24,
  [KEY_Z] = 25,
};

int main(int argc, char **argv) {
  int fd = open("/dev/input/event0", O_RDWR);
  if (fd < 0) {
    perror("cant open event0");
  }
  assert(fd >= 0);
  for (int i=0; i<26; i++) {
    keys[i] = ' ';
  }
  keys[26] = 0;

  int ret = ioctl(fd, EVIOCGRAB, (void*)1);
  assert(ret == 0);

  struct input_event ev[64];
  int x = 0;
  int y = 0;
  bool mouse_pending = false;
  while (1) {
    int rd = read(fd, ev, sizeof(ev));
    //printf("got %d/%d %d\n", rd, sizeof(ev[0]), sizeof(struct input_event_compat));
    for (int j=0; j < rd / ((signed int)sizeof(struct input_event_compat)); j++) {
      if (ev[j].type == EV_ABS) {
        if (ev[j].code == ABS_X) x = ev[j].value;
        else if (ev[j].code == ABS_Y) y = ev[j].value;
        mouse_pending = true;
      } else if (ev[j].type == EV_SYN) {
        if (mouse_pending) {
          printf("%s %3d %3d\r", keys, x, y);
          fflush(stdout);
          mouse_pending = false;
        }
      } else if (ev[j].type == EV_KEY) {
        if ((ev[j].code < sizeof(mapping)) && ((mapping[ev[j].code]) || (ev[j].code == KEY_A))) {
          int index = mapping[ev[j].code];
          char ascii = 'A' + index;
          switch (ev[j].value) {
          case 0:
            keys[index] = ' ';
            break;
          case 1:
            keys[index] = ascii;
            break;
          case 2:
            keys[index] = ascii + ('a' - 'A');
            break;
          }
          mouse_pending = true;
        }
      } else {
        printf("sec: %ld.%06ld type: %d code: %d value: %d\n", ev[j].input_event_sec, ev[j].input_event_usec, ev[j].type, ev[j].code, ev[j].value);
      }
    }
  }
}
