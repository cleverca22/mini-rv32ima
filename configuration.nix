{ pkgs, ... }:

{
  imports = [
  ];
  initrd.packages = [
    (pkgs.callPackage ./evtest.nix {})
  ];
  nixpkgs.overlays = [ (self: super: {
    ubootTools = null;
  }) ];
}
