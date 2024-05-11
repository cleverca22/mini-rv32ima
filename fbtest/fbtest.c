#include <assert.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

static void fill(uint32_t color, uint32_t *fb, int w, int h) {
  color |= (0xff << 0);
  for (int i=0; i<(w * h); i++) {
    fb[i] = color;
  }
  printf("0x%08x\n", color);
}

int main(int argc, char **argv) {
  int fd = open("/dev/fb0", O_RDWR);
  assert(fd);
  struct fb_var_screeninfo fb = {};
  /* fetch framebuffer info */
  ioctl(fd, FBIOGET_VSCREENINFO, &fb);
  printf("framebuffer: x_res: %d, y_res: %d, x_virtual: %d, y_virtual: %d, bpp: %d, grayscale: %d\n",
            fb.xres, fb.yres, fb.xres_virtual, fb.yres_virtual, fb.bits_per_pixel, fb.grayscale);

  printf("framebuffer: RGBA: %d%d%d%d, red_off: %d, green_off: %d, blue_off: %d, transp_off: %d\n",
            fb.red.length, fb.green.length, fb.blue.length, fb.transp.length, fb.red.offset, fb.green.offset, fb.blue.offset, fb.transp.offset);


  int bytes = fb.bits_per_pixel * fb.xres * fb.yres / 8;
  uint32_t *buffer = malloc(bytes);

  fb.red.offset = 24;
  fb.green.offset = 16;
  fb.blue.offset = 8;
  fb.transp.offset = 0;

  if (argc == 2) {
    printf("%s\n", argv[1]);
    uint32_t color = strtol(argv[1], NULL, 16);
    fill(color, buffer, fb.xres, fb.yres);
    pwrite(fd, buffer, bytes, 0);
    printf("0x%08x\n", color);
  } else {
    int delay = 5;

    fill(0xff << fb.red.offset, buffer, fb.xres, fb.yres);
    pwrite(fd, buffer, bytes, 0);
    puts("red");
    sleep(delay);

    fill(0xff << fb.green.offset, buffer, fb.xres, fb.yres);
    pwrite(fd, buffer, bytes, 0);
    puts("green");
    sleep(delay);

    fill(0xff << fb.blue.offset, buffer, fb.xres, fb.yres);
    pwrite(fd, buffer, bytes, 0);
    puts("blue");
    sleep(delay);
  }
}
