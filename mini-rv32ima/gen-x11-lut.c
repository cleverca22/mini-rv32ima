#define XK_MISCELLANY

#include <X11/keysymdef.h>
#include <assert.h>
#include <linux/input.h>
#include <stdint.h>
#include <stdio.h>

struct key_entry {
  int linux_code;
  char *linux_name;
};

const struct key_entry x11_table[65536] = {
  [13] = { .linux_code = KEY_ENTER, .linux_name = "KEY_ENTER" },
  [32] = { .linux_code = KEY_SPACE, .linux_name = "KEY_SPACE" },
  [39] = { .linux_code = KEY_LEFTCTRL, .linux_name = "KEY_LEFTCTRL" },
  [44] = { .linux_code = KEY_COMMA, .linux_name = "KEY_COMMA" },
  [45] = { .linux_code = KEY_MINUS, .linux_name = "KEY_MINUS" },
  [46] = { .linux_code = KEY_DOT, .linux_name = "KEY_DOT" },
  [47] = { .linux_code = KEY_SLASH, .linux_name = "KEY_SLASH" },
  [48] = { .linux_code = KEY_0, .linux_name = "KEY_0" },
  [49] = { .linux_code = KEY_1, .linux_name = "KEY_1" },
  [50] = { .linux_code = KEY_2, .linux_name = "KEY_2" },
  [51] = { .linux_code = KEY_3, .linux_name = "KEY_3" },
  [52] = { .linux_code = KEY_4, .linux_name = "KEY_4" },
  [53] = { .linux_code = KEY_5, .linux_name = "KEY_5" },
  [54] = { .linux_code = KEY_6, .linux_name = "KEY_6" },
  [55] = { .linux_code = KEY_7, .linux_name = "KEY_7" },
  [56] = { .linux_code = KEY_8, .linux_name = "KEY_8" },
  [57] = { .linux_code = KEY_9, .linux_name = "KEY_9" },
  [59] = { .linux_code = KEY_SEMICOLON, .linux_name = "KEY_SEMICOLON" },
  [61] = { .linux_code = KEY_EQUAL, .linux_name = "KEY_EQUAL" },
  [65] = { .linux_code = KEY_A, .linux_name = "KEY_A" },
  [66] = { .linux_code = KEY_B, .linux_name = "KEY_B" },
  [91] = { .linux_code = KEY_LEFTBRACE, .linux_name = "KEY_LEFTBRACE" },
  [92] = { .linux_code = KEY_BACKSLASH, .linux_name = "KEY_BACKSLASH" },
  [93] = { .linux_code = KEY_RIGHTBRACE, .linux_name = "KEY_RIGHTBRACE" },
  [96] = { .linux_code = KEY_GRAVE, .linux_name = "KEY_GRAVE" },
  [97] = { .linux_code = KEY_A, .linux_name = "KEY_A" },
  [98] = { .linux_code = KEY_B, .linux_name = "KEY_B" },
  [99] = { .linux_code = KEY_C, .linux_name = "KEY_C" },
  [100] = { .linux_code = KEY_D, .linux_name = "KEY_D" },
  [101] = { .linux_code = KEY_E, .linux_name = "KEY_E" },
  [102] = { .linux_code = KEY_F, .linux_name = "KEY_F" },
  [103] = { .linux_code = KEY_G, .linux_name = "KEY_G" },
  [104] = { .linux_code = KEY_H, .linux_name = "KEY_H" },
  [105] = { .linux_code = KEY_I, .linux_name = "KEY_I" },
  [106] = { .linux_code = KEY_J, .linux_name = "KEY_J" },
  [107] = { .linux_code = KEY_K, .linux_name = "KEY_K" },
  [108] = { .linux_code = KEY_L, .linux_name = "KEY_L" },
  [109] = { .linux_code = KEY_M, .linux_name = "KEY_M" },
  [110] = { .linux_code = KEY_N, .linux_name = "KEY_N" },
  [111] = { .linux_code = KEY_O, .linux_name = "KEY_O" },
  [112] = { .linux_code = KEY_P, .linux_name = "KEY_P" },
  [113] = { .linux_code = KEY_Q, .linux_name = "KEY_Q" },
  [114] = { .linux_code = KEY_R, .linux_name = "KEY_R" },
  [115] = { .linux_code = KEY_S, .linux_name = "KEY_S" },
  [116] = { .linux_code = KEY_T, .linux_name = "KEY_T" },
  [117] = { .linux_code = KEY_U, .linux_name = "KEY_U" },
  [118] = { .linux_code = KEY_V, .linux_name = "KEY_V" },
  [119] = { .linux_code = KEY_W, .linux_name = "KEY_W" },
  [120] = { .linux_code = KEY_X, .linux_name = "KEY_X" },
  [121] = { .linux_code = KEY_Y, .linux_name = "KEY_Y" },
  [122] = { .linux_code = KEY_Z, .linux_name = "KEY_Z" },
// TODO, replace more numbers with the constants from the x11 header
  [XK_BackSpace] = { .linux_code = KEY_BACKSPACE, .linux_name = "KEY_BACKSPACE" },
  [XK_Tab] = { .linux_code = KEY_TAB, .linux_name = "KEY_TAB" },
  [XK_Return] = { .linux_code = KEY_ENTER, .linux_name = "KEY_ENTER" },
  [XK_Pause] = { .linux_code = KEY_PAUSE, .linux_name = "KEY_PAUSE" },
  [XK_Scroll_Lock] = { .linux_code = KEY_SCROLLLOCK, .linux_name = "KEY_SCROLLLOCK" },
  [XK_Escape] = { .linux_code = KEY_ESC, .linux_name = "KEY_ESC" },
  [XK_Home] = { .linux_code = KEY_HOME, .linux_name = "KEY_HOME" },
  [0xff51] = { .linux_code = KEY_LEFT, .linux_name = "KEY_LEFT" },
  [0xff52] = { .linux_code = KEY_UP, .linux_name = "KEY_UP" },
  [0xff53] = { .linux_code = KEY_RIGHT, .linux_name = "KEY_RIGHT" },
  [0xff54] = { .linux_code = KEY_DOWN, .linux_name = "KEY_DOWN" },
  [0xff55] = { .linux_code = KEY_PAGEUP, .linux_name = "KEY_PAGEUP" },
  [0xff56] = { .linux_code = KEY_PAGEDOWN, .linux_name = "KEY_PAGEDOWN" },
  [0xff57] = { .linux_code = KEY_END, .linux_name = "KEY_END" },
  [0xff63] = { .linux_code = KEY_INSERT, .linux_name = "KEY_INSERT" },
  [0xff67] = { .linux_code = KEY_COMPOSE, .linux_name = "KEY_COMPOSE" },
  [0xff7f] = { .linux_code = KEY_NUMLOCK, .linux_name = "KEY_NUMLOCK" },
  [0xff8d] = { .linux_code = KEY_KPENTER, .linux_name = "KEY_KPENTER" },
  [0xff95] = { .linux_code = KEY_KP7, .linux_name = "KEY_KP7" },
  [0xff96] = { .linux_code = KEY_KP4, .linux_name = "KEY_KP4" },
  [0xff97] = { .linux_code = KEY_KP8, .linux_name = "KEY_KP8" },
  [0xff98] = { .linux_code = KEY_KP6, .linux_name = "KEY_KP6" },
  [0xff99] = { .linux_code = KEY_KP2, .linux_name = "KEY_KP2" },
  [0xff9a] = { .linux_code = KEY_KP9, .linux_name = "KEY_KP9" },
  [0xff9b] = { .linux_code = KEY_KP3, .linux_name = "KEY_KP3" },
  [0xff9c] = { .linux_code = KEY_KP1, .linux_name = "KEY_KP1" },
  [0xff9d] = { .linux_code = KEY_KP5, .linux_name = "KEY_KP5" },
  [0xff9e] = { .linux_code = KEY_KP0, .linux_name = "KEY_KP0" },
  [0xff9f] = { .linux_code = KEY_KPDOT, .linux_name = "KEY_KPDOT" },
  [0xffaa] = { .linux_code = KEY_KPASTERISK, .linux_name = "KEY_KPASTERISK" },
  [0xffab] = { .linux_code = KEY_KPPLUS, .linux_name = "KEY_KPPLUS" },
  [0xffad] = { .linux_code = KEY_KPMINUS, .linux_name = "KEY_KPMINUS" },
  [0xffaf] = { .linux_code = KEY_KPSLASH, .linux_name = "KEY_KPSLASH" },
  [0xffbe] = { .linux_code = KEY_F1, .linux_name = "KEY_F1" },
  [0xffbf] = { .linux_code = KEY_F2, .linux_name = "KEY_F2" },
  [0xffc0] = { .linux_code = KEY_F3, .linux_name = "KEY_F3" },
  [0xffc1] = { .linux_code = KEY_F4, .linux_name = "KEY_F4" },
  [0xffc2] = { .linux_code = KEY_F5, .linux_name = "KEY_F5" },
  [0xffc3] = { .linux_code = KEY_F6, .linux_name = "KEY_F6" },
  [0xffc4] = { .linux_code = KEY_F7, .linux_name = "KEY_F7" },
  [0xffc5] = { .linux_code = KEY_F8, .linux_name = "KEY_F8" },
  [0xffc6] = { .linux_code = KEY_F9, .linux_name = "KEY_F9" },
  [0xffc7] = { .linux_code = KEY_F10, .linux_name = "KEY_F10" },
  [0xffc8] = { .linux_code = KEY_F11, .linux_name = "KEY_F11" },
  [0xffc9] = { .linux_code = KEY_F12, .linux_name = "KEY_F12" },
  [0xffe1] = { .linux_code = KEY_LEFTSHIFT, .linux_name = "KEY_LEFTSHIFT" },
  [0xffe2] = { .linux_code = KEY_RIGHTSHIFT, .linux_name = "KEY_RIGHTSHIFT" },
  [0xffe3] = { .linux_code = KEY_LEFTCTRL, .linux_name = "KEY_LEFTCTRL" },
  [0xffe5] = { .linux_code = KEY_CAPSLOCK, .linux_name = "KEY_CAPSLOCK" },
  [0xffe9] = { .linux_code = KEY_LEFTALT, .linux_name = "KEY_LEFTALT" },
  [0xffea] = { .linux_code = KEY_RIGHTALT, .linux_name = "KEY_RIGHTALT" },
  [0xffeb] = { .linux_code = KEY_LEFTMETA, .linux_name = "KEY_LEFTMETA" },
  [0xffec] = { .linux_code = KEY_RIGHTMETA, .linux_name = "KEY_RIGHTMETA" },
  [XK_Delete] = { .linux_code = KEY_DELETE, .linux_name = "KEY_DELETE" },
};

uint8_t key_bitmap[16];

int main(int argc, char **argv) {
  puts("// auto-generated by gen-x11-lut.c");
  puts("static uint16_t translate_keycode(int keycode) {");
  puts("  switch (keycode) {");
  for (int i=0; i< (sizeof(x11_table) / sizeof(x11_table[0])); i++) {
    if (x11_table[i].linux_name == 0) continue;
    printf("  case 0x%x:\n", i);
    printf("    return %s;\n", x11_table[i].linux_name);
  }
  puts("  }");
  puts("  return 0;");
  puts("}");
  for (int i=0; i < (sizeof(x11_table)/sizeof(x11_table[0])); i++) {
    if (x11_table[i].linux_code) {
      unsigned int code = x11_table[i].linux_code;
      unsigned int byte = code / 8;
      unsigned int bit = code % 8;
      //printf("rawdraw:%u linux:%u byte:%u bit:%u\n", i, code, byte, bit);
      assert(byte < sizeof(key_bitmap));
      key_bitmap[byte] |= 1<<bit;
    }
  }
  puts("static uint8_t valid_keys_bitmap[16] = {");
  for (int i=0; i<sizeof(key_bitmap); i++) {
    printf("  [%d] = 0x%x,\n", i, key_bitmap[i]);
  }
  puts("};");
}
