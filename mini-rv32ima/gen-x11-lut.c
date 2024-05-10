#define XK_MISCELLANY
#define XK_LATIN1

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
  // http
  [0x0d] = { .linux_code = KEY_ENTER, .linux_name = "KEY_ENTER" },
  [0x10] = { .linux_code = KEY_RIGHTSHIFT, .linux_name = "KEY_RIGHTSHIFT" },
  [0x11] = { .linux_code = KEY_LEFTCTRL, .linux_name = "KEY_LEFTCTRL" },
  [0x1b] = { .linux_code = KEY_ESC, .linux_name = "KEY_ESC" },
  [0x25] = { .linux_code = KEY_LEFT, .linux_name = "KEY_LEFT" },
  [0x26] = { .linux_code = KEY_UP, .linux_name = "KEY_UP" },
  [0x27] = { .linux_code = KEY_RIGHT, .linux_name = "KEY_RIGHT" },
  [0x28] = { .linux_code = KEY_DOWN, .linux_name = "KEY_DOWN" },

  // x11
  [XK_0] = { .linux_code = KEY_0, .linux_name = "KEY_0" },
  [XK_1] = { .linux_code = KEY_1, .linux_name = "KEY_1" },
  [XK_2] = { .linux_code = KEY_2, .linux_name = "KEY_2" },
  [XK_3] = { .linux_code = KEY_3, .linux_name = "KEY_3" },
  [XK_4] = { .linux_code = KEY_4, .linux_name = "KEY_4" },
  [XK_5] = { .linux_code = KEY_5, .linux_name = "KEY_5" },
  [XK_6] = { .linux_code = KEY_6, .linux_name = "KEY_6" },
  [XK_7] = { .linux_code = KEY_7, .linux_name = "KEY_7" },
  [XK_8] = { .linux_code = KEY_8, .linux_name = "KEY_8" },
  [XK_9] = { .linux_code = KEY_9, .linux_name = "KEY_9" },
  [XK_A] = { .linux_code = KEY_A, .linux_name = "KEY_A" },
  [XK_B] = { .linux_code = KEY_B, .linux_name = "KEY_B" },
  [XK_BackSpace] = { .linux_code = KEY_BACKSPACE, .linux_name = "KEY_BACKSPACE" },
  [XK_Delete] = { .linux_code = KEY_DELETE, .linux_name = "KEY_DELETE" },
  [XK_Escape] = { .linux_code = KEY_ESC, .linux_name = "KEY_ESC" },
  [XK_Home] = { .linux_code = KEY_HOME, .linux_name = "KEY_HOME" },
  [XK_Pause] = { .linux_code = KEY_PAUSE, .linux_name = "KEY_PAUSE" },
  [XK_Return] = { .linux_code = KEY_ENTER, .linux_name = "KEY_ENTER" },
  [XK_Scroll_Lock] = { .linux_code = KEY_SCROLLLOCK, .linux_name = "KEY_SCROLLLOCK" },
  [XK_Tab] = { .linux_code = KEY_TAB, .linux_name = "KEY_TAB" },
  [XK_a] = { .linux_code = KEY_A, .linux_name = "KEY_A" },
  [XK_b] = { .linux_code = KEY_B, .linux_name = "KEY_B" },
  [XK_backslash] = { .linux_code = KEY_BACKSLASH, .linux_name = "KEY_BACKSLASH" },
  [XK_bracketleft] = { .linux_code = KEY_LEFTBRACE, .linux_name = "KEY_LEFTBRACE" },
  [XK_bracketright] = { .linux_code = KEY_RIGHTBRACE, .linux_name = "KEY_RIGHTBRACE" },
  [XK_c] = { .linux_code = KEY_C, .linux_name = "KEY_C" },
  [XK_Down] = { .linux_code = KEY_DOWN, .linux_name = "KEY_DOWN" },
  [XK_Left] = { .linux_code = KEY_LEFT, .linux_name = "KEY_LEFT" },
  [XK_Right] = { .linux_code = KEY_RIGHT, .linux_name = "KEY_RIGHT" },
  [XK_Up] = { .linux_code = KEY_UP, .linux_name = "KEY_UP" },
  [XK_comma] = { .linux_code = KEY_COMMA, .linux_name = "KEY_COMMA" },
  [XK_d] = { .linux_code = KEY_D, .linux_name = "KEY_D" },
  [XK_e] = { .linux_code = KEY_E, .linux_name = "KEY_E" },
  [XK_equal] = { .linux_code = KEY_EQUAL, .linux_name = "KEY_EQUAL" },
  [XK_f] = { .linux_code = KEY_F, .linux_name = "KEY_F" },
  [XK_g] = { .linux_code = KEY_G, .linux_name = "KEY_G" },
  [XK_grave] = { .linux_code = KEY_GRAVE, .linux_name = "KEY_GRAVE" },
  [XK_h] = { .linux_code = KEY_H, .linux_name = "KEY_H" },
  [XK_i] = { .linux_code = KEY_I, .linux_name = "KEY_I" },
  [XK_j] = { .linux_code = KEY_J, .linux_name = "KEY_J" },
  [XK_k] = { .linux_code = KEY_K, .linux_name = "KEY_K" },
  [XK_l] = { .linux_code = KEY_L, .linux_name = "KEY_L" },
  [XK_m] = { .linux_code = KEY_M, .linux_name = "KEY_M" },
  [XK_minus] = { .linux_code = KEY_MINUS, .linux_name = "KEY_MINUS" },
  [XK_n] = { .linux_code = KEY_N, .linux_name = "KEY_N" },
  [XK_o] = { .linux_code = KEY_O, .linux_name = "KEY_O" },
  [XK_p] = { .linux_code = KEY_P, .linux_name = "KEY_P" },
  [XK_period] = { .linux_code = KEY_DOT, .linux_name = "KEY_DOT" },
  [XK_q] = { .linux_code = KEY_Q, .linux_name = "KEY_Q" },
  [XK_r] = { .linux_code = KEY_R, .linux_name = "KEY_R" },
  [XK_s] = { .linux_code = KEY_S, .linux_name = "KEY_S" },
  [XK_semicolon] = { .linux_code = KEY_SEMICOLON, .linux_name = "KEY_SEMICOLON" },
  [XK_slash] = { .linux_code = KEY_SLASH, .linux_name = "KEY_SLASH" },
  [XK_space] = { .linux_code = KEY_SPACE, .linux_name = "KEY_SPACE" },
  [XK_t] = { .linux_code = KEY_T, .linux_name = "KEY_T" },
  [XK_u] = { .linux_code = KEY_U, .linux_name = "KEY_U" },
  [XK_v] = { .linux_code = KEY_V, .linux_name = "KEY_V" },
  [XK_w] = { .linux_code = KEY_W, .linux_name = "KEY_W" },
  [XK_x] = { .linux_code = KEY_X, .linux_name = "KEY_X" },
  [XK_y] = { .linux_code = KEY_Y, .linux_name = "KEY_Y" },
  [XK_z] = { .linux_code = KEY_Z, .linux_name = "KEY_Z" },
// TODO, replace more numbers with the constants from the x11 header
  [XK_End] = { .linux_code = KEY_END, .linux_name = "KEY_END" },
  [XK_Insert] = { .linux_code = KEY_INSERT, .linux_name = "KEY_INSERT" },
  [XK_Page_Down] = { .linux_code = KEY_PAGEDOWN, .linux_name = "KEY_PAGEDOWN" },
  [XK_Page_Up] = { .linux_code = KEY_PAGEUP, .linux_name = "KEY_PAGEUP" },
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
