{ pkgs, config, ... }:

{
  config = {
    system.build.toplevel = pkgs.runCommand "toplevel" {
      passthru = {
        inherit (config.system.build) kernel initrd shrunkenBinaries;
      };
    } ''
      mkdir $out
      cd $out
      cp ${config.system.build.kernel}/Image .
      cp ${config.system.build.initrd}/initrd .
      cp ${config.system.build.kernel.configfile} kernel.config
    '';
  };
}
