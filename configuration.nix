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
  }) ];
}
