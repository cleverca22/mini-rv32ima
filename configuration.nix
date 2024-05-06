{ pkgs, ... }:

{
  initrd.packages = [
    (pkgs.callPackage ./evtest.nix {})
    (pkgs.callPackage ./fbdoom.nix {})
  ];
  nixpkgs.overlays = [ (self: super: {
    ubootTools = null;
  }) ];
}
