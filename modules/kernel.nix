{ pkgs, lib, config, ... }:

with lib;

let
  kernel = pkgs.linux_latest.override {
    enableCommonConfig = false;
    autoModules = false;
    structuredExtraConfig = with lib.kernel; {
      #MODULES = no; # TODO, breaks the nix build
      ARCH_RV32I = yes;
      BINFMT_ELF_FDPIC = yes;
      BLOCK = no;
      BPF_SYSCALL = no;
      CGROUPS = no;
      CMODEL_MEDLOW = yes;
      COREDUMP = no;
      CPU_FREQ = no;
      #CRYPTO = no;
      CRYPTO_BLAKE2B = no;
      DRM = no;
      FB = no;
      FPU = no;
      IKCONFIG = no;
      LIBCRC32C = no;
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
      SPI = no;
      SUSPEND = no;
      USB_SUPPORT = no;
      VIRTIO_MENU = no;
      SOUND = no;
      HID_SUPPORT = no;
    } // lib.optionalAttrs config.kernel.bake_in_initrd {
      INITRAMFS_SOURCE = freeform "${config.system.build.initrd}/initrd.cpio";
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