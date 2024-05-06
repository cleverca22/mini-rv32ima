{ pkgs, ... }:

{
  imports = [
    ./configuration-fbdoom.nix
  ];
  initrd.packages = [
    (pkgs.callPackage ./evtest.nix {})
  ];
  nixpkgs.overlays = [ (self: super: {
    ubootTools = null;
  }) ];
}
