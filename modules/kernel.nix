{ pkgs, lib, config, ... }:

with lib;

let
  cfg = config.kernel;
  virtio = true;
  block_support = true;
  kernel = pkgs.linux_latest.override {
    enableCommonConfig = false;
    autoModules = false;
    structuredExtraConfig = with lib.kernel; {
      #CRYPTO = no;
      #MODULES = no; # TODO, breaks the nix build
      ARCH_RV32I = yes;
      BINFMT_ELF_FDPIC = yes;
      BLOCK = if block_support then yes else no;
      BPF_SYSCALL = no;
      CGROUPS = no;
      CLK_STARFIVE_JH7110_SYS = no;
      CMODEL_MEDLOW = yes;
      COREDUMP = no;
      CPU_FREQ = no;
      CRYPTO_BLAKE2B = if block_support then yes else no;
      DRM = no;
      FB = yes;
      FB_SIMPLE = yes; # simple-framebuffer
      FPU = no;
      FRAMEBUFFER_CONSOLE = if cfg.fb_console then yes else no; # render text on fb0
      HID_SUPPORT = no;
      IIO = no;
      IKCONFIG = no;
      INPUT_EVDEV = yes; # /dev/input/event0
      INPUT_KEYBOARD = no;
      INPUT_MOUSE = no;
      INPUT_MOUSEDEV = no; # /dev/input/mouseX and /dev/input/mice
      LIBCRC32C = if block_support then yes else no;
      LSM = freeform "";
      MEMTEST = no;
      MMC = no;
      MMU = no;
      MTD = no;
      NAMESPACES = no;
      NET = no;
      NETFILTER = no;
      NLS = no;
      NONPORTABLE = yes;
      PAGE_OFFSET = freeform "0x80000000";
      PCI = no;
      PHYS_RAM_BASE = freeform "0x80000000";
      PHYS_RAM_BASE_FIXED = yes;
      POWER_SUPPLY = no;
      PROFILING = no;
      RD_BZIP2 = no;
      RD_GZIP = if cfg.gzip_initrd then yes else no;
      RD_LZ4 = no;
      RD_LZMA = no;
      RD_LZO = no;
      RD_XZ = no;
      RD_ZSTD = no;
      REGULATOR = no;
      RESET_POLARFIRE_SOC = no;
      RESET_STARFIVE_JH7100 = no;
      RISCV_ISA_C = no; # mini-rv32ima lacks support for the c extensions
      RISCV_ISA_FALLBACK = no;
      SERIAL_8250 = yes; # TODO, remove
      SERIAL_AMBA_PL011 = yes;
      SERIAL_AMBA_PL011_CONSOLE = yes;
      SERIAL_EARLYCON = yes;
      SERIAL_OF_PLATFORM = yes;
      SERIO = no;
      SMP = no;
      SND = no;
      SOUND = no;
      SPI = no;
      SUSPEND = no;
      USB_SUPPORT = no;
      VHOST_MENU = no;
      VIRTIO_MENU = no;
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
    kernel = {
      bake_in_initrd = mkOption {
        default = false;
        type = types.bool;
      };
      fb_console = mkOption {
        default = true;
        type = types.bool;
      };
      gzip_initrd = mkOption {
        default = true;
        type = types.bool;
      };
    };
  };
  config = {
    system.build.kernel = kernel;
  };
}
