{ lib, stdenv, dtc, xorg, mesa, http ? false }:

stdenv.mkDerivation {
  src = ./.;
  name = "mini-rv32ima";
  makeFlags = [
    "mini-rv32ima full-rv32ima"
  ] ++ lib.optional http "HTTP=1";
  installPhase = ''
    mkdir -p $out/bin
    cp mini-rv32ima{,.tiny} full-rv32ima $out/bin/
  '';
  postFixup = ''
    rm -rf $out/nix-support
  '';
  # dontStrip = true;
  STATIC = stdenv.hostPlatform.isStatic;
  buildInputs = [ dtc ] ++ lib.optional stdenv.hostPlatform.isLinux xorg.libX11;
}
