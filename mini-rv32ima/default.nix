{ lib, stdenv, dtc, xorg, mesa, http ? false, buildPackages, linuxHeaders, alsa-lib, windows }:

stdenv.mkDerivation {
  src = ./.;
  name = "mini-rv32ima";
  makeFlags = [
    "mini-rv32ima full-rv32ima"
  ] ++ lib.optional http "HTTP=1"
  ++ lib.optionals stdenv.hostPlatform.isLinux [ "LINUX=1" ]
  ++ lib.optionals stdenv.hostPlatform.isWindows [ "WINDOWS=1" ];
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
  buildInputs = [ dtc ]
    ++ lib.optionals stdenv.hostPlatform.isLinux [ xorg.libX11 alsa-lib ]
    ++ lib.optionals stdenv.hostPlatform.isWindows [ linuxHeaders windows.mingw_w64_pthreads ];
  enableParallelBuilding = true;
}
