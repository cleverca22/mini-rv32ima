{ pkgs, ... }:

{
  initrd = {
    packages = [
      (pkgs.callPackage ./fbdoom.nix {})
    ];
    postInit = "fbdoom -mb 2 -iwad /doom2.wad > /dev/ttyAMA0 2>&1"; # -playdemo demo.lmp";
  };
  kernel.fb_console = false;
}
