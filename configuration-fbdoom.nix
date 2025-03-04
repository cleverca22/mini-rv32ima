{ pkgs, ... }:

{
  initrd = {
    packages = [
      (pkgs.callPackage ./fbdoom.nix {})
    ];
    postInit = ''
      ln -sv ${./DOOM2.WAD} /doom2.wad
      ln -sv ${./CS01-009.LMP} /demo.lmp
    '';
    inittab = "console::respawn:fbdoom -mb 2 -iwad /doom2.wad > /dev/ttyAMA0 2>&1";
    # -playdemo demo.lmp";
    shellOnTTY1 = false;
  };
  kernel = {
    block = false;
    fb_console = false;
    network = false;
    evdev = true; # used by fbdoom for all input
  };
}
