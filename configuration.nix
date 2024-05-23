{ pkgs, ... }:

{
  imports = [
  ];
  initrd.packages = [
    (pkgs.callPackage ./evtest.nix {})
    (pkgs.callPackage ./fbtest {})
    pkgs.evtest
  ];
  nixpkgs.overlays = [ (self: super: {
    ubootTools = null;
    cnfa = self.callPackage ./cnfa.nix {};
    alsa-lib = super.alsa-lib.overrideAttrs (old: {
      configureFlags = [
        "--disable-mixer"
        "--enable-static"
        "--disable-shared"
        "--with-pcm-plugins=copy,linear"
        #"--help"
      ];
    });
  }) ];
  #initrd.inittab = "ttyAMA0::respawn:poweroff";
  kernel = {
    gfx = true;
  };
  initrd.postInit = ''
    ip link set eth0 up
    ip addr add 10.0.0.100/24 dev eth0
    ip route add via 10.0.0.1 dev eth0
  '';
}
