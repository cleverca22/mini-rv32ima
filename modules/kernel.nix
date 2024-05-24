{ pkgs, lib, config, ... }:

with lib;

let
  cfg = config.kernel;
  base = with lib.kernel; {
    #CRYPTO = no;
    #MODULES = no; # TODO, breaks the nix build
    ADVISE_SYSCALLS = no;
    AIO = no;
    ALLOW_DEV_COREDUMP = no;
    ARCH_RV32I = yes;
    BASE_FULL = no;
    BASE_SMALL = yes; # reduces memory usage at the cost of cpu perf
    BINFMT_ELF_FDPIC = yes;
    BINFMT_SCRIPT = yes; # #! support
    BLK_DEV_INITRD = yes; # support having an initrd at all
    BLOCK = no;
    BPF_SYSCALL = no;
    CC_OPTIMIZE_FOR_SIZE = yes;
    CGROUPS = no;
    CHECKPOINT_RESTORE = no;
    CLK_STARFIVE_JH7110_SYS = no;
    CMODEL_MEDLOW = yes;
    CONSOLE_TRANSLATIONS = no;
    COREDUMP = no;
    CPU_FREQ = no;
    DAX = no;
    DEBUG_ATOMIC_SLEEP = no;
    DEBUG_BUGVERBOSE = no;
    DEBUG_KERNEL = no;
    DEBUG_LIST = no;
    DEBUG_MEMORY_INIT = no;
    DEBUG_MISC = no;
    DEBUG_MUTEXES = no;
    DEBUG_PLIST = no;
    DEBUG_RWSEMS = no;
    DEBUG_SG = no;
    DEBUG_SPINLOCK = no;
    DEBUG_TIMEKEEPING = no;
    DEVMEM = no;
    DEVTMPFS = yes;
    DEVTMPFS_MOUNT = yes;
    DMADEVICES = no;
    DRM = no;
    EPOLL = no;
    ETHERNET = no;
    EVENTFD = no;
    EXPERT = yes;
    FHANDLE = no;
    FILE_LOCKING = no;
    FONTS = yes;
    FONT_8x8 = no;
    FPU = no;
    FRAMEBUFFER_CONSOLE = if cfg.fb_console then yes else no; # render text on fb0
    FRAMEBUFFER_CONSOLE_DETECT_PRIMARY = yes;
    FW_LOADER = no;
    GENERIC_PCI_IOMAP = no;
    HID_SUPPORT = no;
    HIGH_RES_TIMERS = no;
    I2C = no;
    IIO = no;
    IKCONFIG = no;
    INPUT_EVDEV = yes; # /dev/input/event0
    INPUT_KEYBOARD = no;
    INPUT_MOUSE = no;
    INPUT_MOUSEDEV = no; # /dev/input/mouseX and /dev/input/mice
    IO_URING = no;
    IPV6 = no;
    KEYS = no;
    LD_DEAD_CODE_DATA_ELIMINATION = yes;
    LSM = freeform "";
    MEMFD_CREATE = no;
    MEMTEST = no;
    MMAP_ALLOW_UNINITIALIZED = yes;
    MMC = no;
    MMU = no;
    MODULES = yes;
    MTD = no;
    NAMESPACES = no;
    NET = no;
    NETFILTER = no;
    NET_9P = no;
    NET_NS = no;
    NLS = no;
    NONPORTABLE = yes;
    NVMEM = no;
    PAGE_OFFSET = freeform "0x80000000";
    PCI = no;
    PERF_EVENTS = no;
    PHYS_RAM_BASE = freeform "0x80000000";
    PHYS_RAM_BASE_FIXED = yes;
    PINCONF = no;
    PINCTRL_STARFIVE_JH7100 = no;
    PINCTRL_STARFIVE_JH7110 = no;
    PINCTRL_STARFIVE_JH7110_AON = no;
    PINCTRL_STARFIVE_JH7110_SYS = no;
    PINMUX = no;
    PM = no;
    POWER_RESET = yes;
    POWER_RESET_SYSCON_POWEROFF = yes;
    POWER_SUPPLY = no;
    PRINTK = if cfg.printk then yes else no;
    PRINTK_TIME = yes;
    PROFILING = no;
    RANDOMIZE_KSTACK_OFFSET = no;
    RCU_EQS_DEBUG = no;
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
    RISCV_EFFICIENT_UNALIGNED_ACCESS = yes;
    RISCV_ISA_C = no; # mini-rv32ima lacks support for the c extensions
    RISCV_ISA_FALLBACK = no;
    RTC_CLASS = no;
    SCHED_DEBUG = no; # 17448 bytes
    SCSI_MOD = no;
    SECCOMP = no;
    SERIAL_8250 = no;
    SERIAL_AMBA_PL011 = yes;
    SERIAL_AMBA_PL011_CONSOLE = yes;
    SERIAL_EARLYCON = yes;
    SERIAL_OF_PLATFORM = yes;
    SERIO = no;
    SIGNALFD = no;
    SLUB_DEBUG = no;
    SLUB_TINY = yes;
    SMP = no;
    SOC_STARFIVE = yes;
    SOC_VIRT = yes;
    SPI = no;
    STACKPROTECTOR = no;
    STACKTRACE = no;
    SUSPEND = no;
    SYNC_FILE = no;
    SYSFS_SYSCALL = no;
    SYSVIPC = no;
    TIMERFD = no;
    USB_SUPPORT = no;
    VHOST_MENU = no;
    VIRTIO_MENU = no;
    VM_EVENT_COUNTERS = no;
    WIRELESS = no;
    WLAN = no;
  };
  snd_cfg = with lib.kernel; {
    SND = yes;
    SOUND = yes;
  };
  block_cfg = with lib.kernel; {
    BLOCK = yes;
    CRYPTO_BLAKE2B = yes;
    LIBCRC32C = yes;
    VIRTIO_BLK = yes;
  };
  gfx_cfg = with lib.kernel; {
    FB = yes;
    FB_SIMPLE = yes; # simple-framebuffer
  };
  net_cfg = with lib.kernel; {
    VIRTIO_NET = yes;
    NETDEVICES = yes;
    IPV6_SIT = no;
    NET_CORE = yes;
    NET = yes;
    INET = yes;
  };
  virtio_cfg = with lib.kernel; {
    VIRTIO_CONSOLE = no; # 25240 bytes
    VIRTIO_INPUT = yes;
    VIRTIO_MENU = yes;
    VIRTIO_MMIO = yes;
  };
  kernel = pkgs.linux_latest.override {
    enableCommonConfig = false;
    autoModules = false;
    defconfig = "allnoconfig";
    structuredExtraConfig = base
    // lib.optionalAttrs config.kernel.bake_in_initrd {
      INITRAMFS_SOURCE = freeform "${config.system.build.initrd}/initrd.cpio";
    }
    // lib.optionalAttrs cfg.gfx gfx_cfg
    // lib.optionalAttrs cfg.virtio virtio_cfg
    // lib.optionalAttrs cfg.block block_cfg
    // lib.optionalAttrs cfg.network net_cfg
    // lib.optionalAttrs cfg.snd snd_cfg
    ;
  };
in {
  options = {
    kernel = {
      bake_in_initrd = mkOption {
        default = false;
        type = types.bool;
      };
      fb_console = mkOption {
        # 32kb
        default = true;
        type = types.bool;
      };
      gzip_initrd = mkOption {
        default = true;
        type = types.bool;
      };
      gfx = mkOption {
        # 91720 bytes
        default = true;
        type = types.bool;
      };
      virtio = mkOption {
        default = true;
        type = types.bool;
      };
      block = mkOption {
        # 1.5mb
        default = false;
        type = types.bool;
      };
      network = mkOption {
        # 1046kb
        default = true;
        type = types.bool;
      };
      snd = mkOption {
        default = false;
        type = types.bool;
      };
      printk = mkOption {
        default = true;
        type = types.bool;
      };
    };
  };
  config = {
    system.build.kernel = kernel;
    kernel.gfx = mkIf cfg.fb_console true;
    kernel.virtio = mkIf cfg.block true;
  };
}
