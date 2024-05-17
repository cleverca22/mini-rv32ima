{ lib, stdenv, dtc, xorg, mesa, http ? false, buildPackages, linuxHeaders, cnfa, alsa-lib }:

stdenv.mkDerivation {
  src = ./.;
  name = "mini-rv32ima";
  makeFlags = [
    "mini-rv32ima full-rv32ima"
  ] ++ lib.optional http "HTTP=1"
  ++ lib.optionals stdenv.hostPlatform.isLinux [ "BLOCK=1" "X11=1" "INPUT=1" "LINUX=1" ]
  ++ lib.optionals stdenv.hostPlatform.isWindows [ "WINDOWS=1" "INPUT=1" ];
  installPhase = let
   ext = stdenv.hostPlatform.extensions.executable;
  in ''
    mkdir -p $out/bin
    cp mini-rv32ima{,-tiny}${ext} full-rv32ima${ext} $out/bin/
  '';
  postFixup = ''
    rm -rf $out/nix-support
  '';
  # dontStrip = true;
  nativeBuildInputs = [ xorg.libX11 ];
  depsBuildBuild = [ buildPackages.stdenv.cc ];
  STATIC = stdenv.hostPlatform.isStatic;
  buildInputs = [ dtc cnfa alsa-lib ] ++ lib.optional stdenv.hostPlatform.isLinux xorg.libX11
    ++ lib.optional stdenv.hostPlatform.isWindows linuxHeaders;
}
