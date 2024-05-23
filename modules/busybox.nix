{ lib, pkgs, config, ... }:
let
  cfg = config.busybox;
  busybox = pkgs.busybox.override {
    enableMinimal = true;
    extraConfig = ''
      # CONFIG_SH_IS_HUSH y
      CONFIG_CAT y
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
      CONFIG_GREP y
      CONFIG_HEAD y
      CONFIG_HUSH y
      CONFIG_HUSH_EXPORT y
      CONFIG_HUSH_INTERACTIVE y
      CONFIG_INIT y
      CONFIG_LN y
      CONFIG_LS y
      CONFIG_MKNOD y
      CONFIG_MKTEMP n
      CONFIG_PS y
      CONFIG_MOUNT y
      CONFIG_PING n
      CONFIG_PING6 n
      CONFIG_POWEROFF y
      CONFIG_REBOOT y
      CONFIG_SHELL_HUSH y
      CONFIG_SH_IS_NONE n
      CONFIG_SLEEP y
      CONFIG_SORT y
      CONFIG_TAIL y
      CONFIG_TRACEROUTE n
      CONFIG_TRACEROUTE6 n
      CONFIG_TTY y
      CONFIG_UDHCPC n
      CONFIG_UNAME y
      CONFIG_UDHCPC6 n
      CONFIG_UDHCPD n
      CONFIG_WC y
      CONFIG_XXD y
      CONFIG_MKDIR y
      CONFIG_TOUCH y
      CONFIG_UMOUNT y
      CONFIG_GETTY y
    '' + (lib.optionalString cfg.free ''
      CONFIG_FREE y
    '')
    + (lib.optionalString true ''
      CONFIG_IP y
      CONFIG_IFCONFIG y
      CONFIG_FEATURE_IFCONFIG_STATUS y
      CONFIG_FEATURE_IP_ADDRESS y
      CONFIG_FEATURE_IP_LINK y
      CONFIG_FEATURE_IP_ROUTE y
      CONFIG_PING y
      CONFIG_WGET y
      CONFIG_FEATURE_WGET_STATUSBAR y
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
