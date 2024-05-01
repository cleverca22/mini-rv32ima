{ pkgs, lib, config, ... }:

with lib;

let
  virtio = true;
  block_support = true;
  kernel = pkgs.linux_latest.override {
    enableCommonConfig = false;
    autoModules = false;
    structuredExtraConfig = with lib.kernel; {
      #MODULES = no; # TODO, breaks the nix build
      ARCH_RV32I = yes;
      BINFMT_ELF_FDPIC = yes;
      BPF_SYSCALL = no;
      CGROUPS = no;
      CMODEL_MEDLOW = yes;
      COREDUMP = no;
      CPU_FREQ = no;
      #CRYPTO = no;
      CRYPTO_BLAKE2B = if block_support then yes else no;
      DRM = no;
      FB = yes;
      FB_SIMPLE = yes; # simple-framebuffer
      FPU = no;
      FRAMEBUFFER_CONSOLE = yes; # render text on fb0
      HID_SUPPORT = no;
      IKCONFIG = no;
      INPUT_EVDEV = yes; # /dev/input/event0
      LIBCRC32C = if block_support then yes else no;
      MMC = no;
      MMU = no;
      MTD = no;
      NAMESPACES = no;
      NET = no;
      NETFILTER = no;
      NONPORTABLE = yes;
      PAGE_OFFSET = freeform "0x80000000";
      PCI = no;
      PHYS_RAM_BASE = freeform "0x80000000";
      PHYS_RAM_BASE_FIXED = yes;
      PROFILING = no;
      RD_BZIP2 = no;
      RD_GZIP = no;
      RD_LZ4 = no;
      RD_LZMA = no;
      RD_LZO = no;
      RD_XZ = no;
      RD_ZSTD = no;
      REGULATOR = no;
      RISCV_ISA_C = no; # mini-rv32ima lacks support for the c extensions
      RISCV_ISA_FALLBACK = no;
      SERIAL_8250 = yes;
      SERIAL_EARLYCON = yes;
      SERIAL_OF_PLATFORM = yes;
      SMP = no;
      SND = no;
      SOUND = no;
      SPI = no;
      SUSPEND = no;
      USB_SUPPORT = no;
      VIRTIO_MENU = no;
      BLOCK = if block_support then yes else no;
    } // lib.optionalAttrs config.kernel.bake_in_initrd {
      INITRAMFS_SOURCE = freeform "${config.system.build.initrd}/initrd.cpio";
    } // lib.optionalAttrs virtio {
      VIRTIO_BLK = if block_support then yes else no;
      VIRTIO_CONSOLE = yes;
      VIRTIO_INPUT = yes;
      VIRTIO_MENU = yes;
      VIRTIO_MMIO = yes;
    };
  };
in {
  options = {
    kernel.bake_in_initrd = mkOption {
      default = false;
      type = types.bool;
    };
  };
  config = {
    system.build.kernel = kernel;
  };
}
