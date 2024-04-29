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
      CONFIG_FEATURE_EDITING_MAX_LEN 1024
      CONFIG_HEAD y
      CONFIG_HUSH y
      CONFIG_PING n
      CONFIG_HUSH_EXPORT y
      CONFIG_HUSH_INTERACTIVE y
      CONFIG_LS y
      CONFIG_MKNOD y
      CONFIG_MKTEMP n
      CONFIG_MOUNT y
      CONFIG_PING6 n
      CONFIG_POWEROFF y
      CONFIG_UNAME y
      CONFIG_REBOOT y
      CONFIG_SHELL_HUSH y
      CONFIG_SH_IS_NONE n
      CONFIG_SLEEP y
      CONFIG_SORT y
      CONFIG_TAIL y
      CONFIG_TRACEROUTE n
      CONFIG_TRACEROUTE6 n
      CONFIG_UDHCPC n
      CONFIG_UDHCPC6 n
      CONFIG_UDHCPD n
      CONFIG_WC y
      CONFIG_XXD y
      CONFIG_MKDIR y
      CONFIG_TOUCH y
      CONFIG_UMOUNT y
      CONFIG_GETTY y
    '' + lib.optionalString cfg.free "CONFIG_FREE y";
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
