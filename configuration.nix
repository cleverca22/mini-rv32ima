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
}
