
// Emulating a 8250 / 16550 UART
static uint32_t uart_load(void *state, uint32_t addr) {
  uint32_t ret = 0;
  switch (addr) {
  case 0:
    if (IsKBHit()) ret = ReadKBByte();
    break;
  case 5:
    ret = 0x60 | IsKBHit();
    break;
  }
  return ret;
}

static void uart_store(void *state, uint32_t addr, uint32_t val) {
  switch (addr) {
  case 0: //UART 8250 / 16550 Data Buffer
    printf("%c", val);
    fflush( stdout );
    break;
  }
}
