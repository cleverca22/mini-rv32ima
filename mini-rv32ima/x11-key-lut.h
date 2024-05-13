// auto-generated by gen-x11-lut.c
static uint16_t translate_keycode(int keycode) {
  switch (keycode) {
  case 0xd:
    return KEY_ENTER;
  case 0x10:
    return KEY_RIGHTSHIFT;
  case 0x11:
    return KEY_LEFTCTRL;
  case 0x1b:
    return KEY_ESC;
  case 0x20:
    return KEY_SPACE;
  case 0x25:
    return KEY_LEFT;
  case 0x26:
    return KEY_UP;
  case 0x27:
    return KEY_RIGHT;
  case 0x28:
    return KEY_DOWN;
  case 0x2c:
    return KEY_COMMA;
  case 0x2d:
    return KEY_MINUS;
  case 0x2e:
    return KEY_DOT;
  case 0x2f:
    return KEY_SLASH;
  case 0x30:
    return KEY_0;
  case 0x31:
    return KEY_1;
  case 0x32:
    return KEY_2;
  case 0x33:
    return KEY_3;
  case 0x34:
    return KEY_4;
  case 0x35:
    return KEY_5;
  case 0x36:
    return KEY_6;
  case 0x37:
    return KEY_7;
  case 0x38:
    return KEY_8;
  case 0x39:
    return KEY_9;
  case 0x3b:
    return KEY_SEMICOLON;
  case 0x3d:
    return KEY_EQUAL;
  case 0x41:
    return KEY_A;
  case 0x42:
    return KEY_B;
  case 0x43:
    return KEY_C;
  case 0x45:
    return KEY_E;
  case 0x46:
    return KEY_F;
  case 0x53:
    return KEY_S;
  case 0x54:
    return KEY_T;
  case 0x5b:
    return KEY_LEFTBRACE;
  case 0x5c:
    return KEY_BACKSLASH;
  case 0x5d:
    return KEY_RIGHTBRACE;
  case 0x60:
    return KEY_GRAVE;
  case 0x61:
    return KEY_A;
  case 0x62:
    return KEY_B;
  case 0x63:
    return KEY_C;
  case 0x64:
    return KEY_D;
  case 0x65:
    return KEY_E;
  case 0x66:
    return KEY_F;
  case 0x67:
    return KEY_G;
  case 0x68:
    return KEY_H;
  case 0x69:
    return KEY_I;
  case 0x6a:
    return KEY_J;
  case 0x6b:
    return KEY_K;
  case 0x6c:
    return KEY_L;
  case 0x6d:
    return KEY_M;
  case 0x6e:
    return KEY_N;
  case 0x6f:
    return KEY_O;
  case 0x70:
    return KEY_P;
  case 0x71:
    return KEY_Q;
  case 0x72:
    return KEY_R;
  case 0x73:
    return KEY_S;
  case 0x74:
    return KEY_T;
  case 0x75:
    return KEY_U;
  case 0x76:
    return KEY_V;
  case 0x77:
    return KEY_W;
  case 0x78:
    return KEY_X;
  case 0x79:
    return KEY_Y;
  case 0x7a:
    return KEY_Z;
  case 0xff08:
    return KEY_BACKSPACE;
  case 0xff09:
    return KEY_TAB;
  case 0xff0d:
    return KEY_ENTER;
  case 0xff13:
    return KEY_PAUSE;
  case 0xff14:
    return KEY_SCROLLLOCK;
  case 0xff1b:
    return KEY_ESC;
  case 0xff50:
    return KEY_HOME;
  case 0xff51:
    return KEY_LEFT;
  case 0xff52:
    return KEY_UP;
  case 0xff53:
    return KEY_RIGHT;
  case 0xff54:
    return KEY_DOWN;
  case 0xff55:
    return KEY_PAGEUP;
  case 0xff56:
    return KEY_PAGEDOWN;
  case 0xff57:
    return KEY_END;
  case 0xff63:
    return KEY_INSERT;
  case 0xff67:
    return KEY_COMPOSE;
  case 0xff7f:
    return KEY_NUMLOCK;
  case 0xff8d:
    return KEY_KPENTER;
  case 0xff95:
    return KEY_KP7;
  case 0xff96:
    return KEY_KP4;
  case 0xff97:
    return KEY_KP8;
  case 0xff98:
    return KEY_KP6;
  case 0xff99:
    return KEY_KP2;
  case 0xff9a:
    return KEY_KP9;
  case 0xff9b:
    return KEY_KP3;
  case 0xff9c:
    return KEY_KP1;
  case 0xff9d:
    return KEY_KP5;
  case 0xff9e:
    return KEY_KP0;
  case 0xff9f:
    return KEY_KPDOT;
  case 0xffaa:
    return KEY_KPASTERISK;
  case 0xffab:
    return KEY_KPPLUS;
  case 0xffad:
    return KEY_KPMINUS;
  case 0xffaf:
    return KEY_KPSLASH;
  case 0xffbe:
    return KEY_F1;
  case 0xffbf:
    return KEY_F2;
  case 0xffc0:
    return KEY_F3;
  case 0xffc1:
    return KEY_F4;
  case 0xffc2:
    return KEY_F5;
  case 0xffc3:
    return KEY_F6;
  case 0xffc4:
    return KEY_F7;
  case 0xffc5:
    return KEY_F8;
  case 0xffc6:
    return KEY_F9;
  case 0xffc7:
    return KEY_F10;
  case 0xffc8:
    return KEY_F11;
  case 0xffc9:
    return KEY_F12;
  case 0xffe1:
    return KEY_LEFTSHIFT;
  case 0xffe2:
    return KEY_RIGHTSHIFT;
  case 0xffe3:
    return KEY_LEFTCTRL;
  case 0xffe5:
    return KEY_CAPSLOCK;
  case 0xffe9:
    return KEY_LEFTALT;
  case 0xffea:
    return KEY_RIGHTALT;
  case 0xffeb:
    return KEY_LEFTMETA;
  case 0xffec:
    return KEY_RIGHTMETA;
  case 0xffff:
    return KEY_DELETE;
  }
  return 0;
}
static uint8_t valid_keys_bitmap[] = {
  [0] = 0xfe,
  [1] = 0xff,
  [2] = 0xff,
  [3] = 0xff,
  [4] = 0xff,
  [5] = 0xfe,
  [6] = 0xff,
  [7] = 0xff,
  [8] = 0xff,
  [9] = 0xff,
  [10] = 0x8f,
  [11] = 0x1,
  [12] = 0xd5,
  [13] = 0xff,
  [14] = 0x80,
  [15] = 0xe0,
  [16] = 0x0,
  [17] = 0x0,
  [18] = 0x0,
  [19] = 0x0,
  [20] = 0x0,
  [21] = 0x0,
  [22] = 0x0,
  [23] = 0x0,
  [24] = 0x0,
  [25] = 0x0,
  [26] = 0x0,
  [27] = 0x0,
  [28] = 0x0,
  [29] = 0x0,
  [30] = 0x0,
  [31] = 0x0,
  [32] = 0x0,
  [33] = 0x0,
  [34] = 0x7,
};
