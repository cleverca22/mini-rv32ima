LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

PLATFORM := mini-rv32ima

GLOBAL_DEFINES += ARCH_RISCV_CLINT_BASE=0x11000000
GLOBAL_DEFINES += ARCH_RISCV_MTIME_RATE=1000000   # 1 MHz