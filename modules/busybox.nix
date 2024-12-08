{ lib, pkgs, config, ... }:
let
  cfg = config.busybox;
  busybox = pkgs.busybox.override {
    enableMinimal = true;
    extraConfig = ''
      # CONFIG_SH_IS_HUSH y
      CONFIG_ASH n
      CONFIG_CAT y
      CONFIG_CP y
      CONFIG_CRC32 y
      CONFIG_DF y
      CONFIG_DHCPRELAY n
      CONFIG_DMESG y
      CONFIG_DU y
      CONFIG_DUMPLEASES n
      CONFIG_ECHO y
      CONFIG_ENV y
      CONFIG_FEATURE_EDITING y
      CONFIG_FEATURE_EDITING_HISTORY 32
      CONFIG_FEATURE_EDITING_MAX_LEN 1024
      CONFIG_FEATURE_USE_INITTAB y
      CONFIG_GETTY y
      CONFIG_GREP y
      CONFIG_HEAD y
      CONFIG_HUSH y
      CONFIG_HUSH_EXPORT y
      CONFIG_HUSH_INTERACTIVE y
      CONFIG_CHVT y
      CONFIG_INIT y
      CONFIG_LN y
      CONFIG_LS y
      CONFIG_MKDIR y
      CONFIG_MKNOD y
      CONFIG_MKPASSWD y
      CONFIG_MKTEMP n
      CONFIG_MOUNT y
      CONFIG_MV y
      CONFIG_NOMMU y
      CONFIG_PIE y
      CONFIG_PING n
      CONFIG_PING6 n
      CONFIG_POWEROFF y
      CONFIG_PS y
      CONFIG_REBOOT y
      CONFIG_SHELL_HUSH y
      CONFIG_SH_IS_NONE n
      CONFIG_SLEEP y
      CONFIG_SORT y
      CONFIG_TAIL y
      CONFIG_TIME y
      CONFIG_TOUCH y
      CONFIG_TRACEROUTE n
      CONFIG_TRACEROUTE6 n
      CONFIG_TTY y
      CONFIG_UDHCPC n
      CONFIG_UDHCPC6 n
      CONFIG_UDHCPD n
      CONFIG_UMOUNT y
      CONFIG_UNAME y
      CONFIG_WC y
      CONFIG_XXD y
    '' + (lib.optionalString cfg.free ''
      CONFIG_FREE y
    '')
    + (lib.optionalString config.kernel.network ''
      CONFIG_FEATURE_FANCY_PING y
      CONFIG_FEATURE_IFCONFIG_STATUS y
      CONFIG_FEATURE_IP_ADDRESS y
      CONFIG_FEATURE_IP_LINK y
      CONFIG_FEATURE_IP_ROUTE y
      CONFIG_FEATURE_WGET_STATUSBAR y
      CONFIG_IFCONFIG y
      CONFIG_IP y
      CONFIG_PING y
      CONFIG_WGET y
    '')
    ;
      #${builtins.readFile ./configs/busybox_config}
  };
  mkEnabledOption = name: (lib.mkEnableOption name) // { default = true; };
in {
  options = {
    busybox = {
      free = mkEnabledOption "free";
    };
  };
  config = {
    system.build.busybox = busybox;
    initrd.packages = [ busybox ];
  };
}
