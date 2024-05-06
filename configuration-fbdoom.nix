{ pkgs, ... }:

{
  initrd.packages = [
    (pkgs.callPackage ./fbdoom.nix {})
  ];
  kernel.fb_console = false;
}
