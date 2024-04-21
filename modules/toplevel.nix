{ pkgs, config, ... }:

{
  config = {
    system.build.toplevel = pkgs.runCommand "toplevel" {} ''
      mkdir $out
      cd $out
      cp ${config.system.build.kernel}/Image .
      cp ${config.system.build.initrd}/initrd .
    '';
  };
}
