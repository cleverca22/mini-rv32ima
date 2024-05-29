pkgs:
{
  inherit (pkgs) mini-rv32ima cnfa qemu curl nix;
  static-rv32ima = pkgs.pkgsStatic.mini-rv32ima;
  windows-rv32ima = pkgs.pkgsCross.mingwW64.mini-rv32ima;
  static-http = pkgs.pkgsStatic.mini-rv32ima.override { http=true; };
  os = (pkgs.callPackage ./os.nix { }).toplevel;
}
