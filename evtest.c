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

int main(int argc, char **argv) {
  int fd = open("/dev/input/event0", O_RDWR);
  if (fd < 0) {
    perror("cant open event0");
  }
  assert(fd >= 0);

  //int ret = ioctl(fd, EVIOCGRAB, (void*)1);
  //assert(ret == 0);

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
          printf("%3d %3d\r", x, y);
          fflush(stdout);
          mouse_pending = false;
        }
      } else {
        printf("sec: %ld.%06ld type: %d code: %d value: %d\n", ev[j].input_event_sec, ev[j].input_event_usec, ev[j].type, ev[j].code, ev[j].value);
      }
    }
  }
}
