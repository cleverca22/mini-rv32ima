{ pkgs, ... }:

{
  imports = [
  ];
  initrd.packages = [
    (pkgs.callPackage ./evtest.nix {})
    pkgs.evtest
  ];
  nixpkgs.overlays = [ (self: super: {
    ubootTools = null;
  }) ];
}
