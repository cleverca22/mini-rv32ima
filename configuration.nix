{ pkgs, ... }:

let
  test = pkgs.writeScript "dotest" ''
    #!/bin/sh
    echo $0
    ping 8.8.8.8
    poweroff
  '';
in
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
  #initrd.inittab = "ttyAMA0::once:${test}";
  kernel = {
    gfx = true;
    block = false;
    gzip_initrd = false;
  };
  initrd.postInit = ''
    cat /proc/net/pnp
    grep nameserver /proc/net/pnp > /etc/resolv.conf
  '';
}
