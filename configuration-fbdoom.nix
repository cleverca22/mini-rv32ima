{ pkgs, ... }:

{
  initrd = {
    packages = [
      (pkgs.callPackage ./fbdoom.nix {})
    ];
    postInit = "fbdoom -mb 2 -iwad /doom2.wad"; # -playdemo demo.lmp";
  };
  kernel.fb_console = false;
}
