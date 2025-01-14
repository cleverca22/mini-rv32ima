{ stdenv, fetchFromGitHub, lib, cnfa }:

stdenv.mkDerivation {
  name = "fbdoom";
  src = fetchFromGitHub {
    owner = "maximevince";
    repo = "fbDOOM";
    rev = "912c51c4d0e5aa899bb534e8c77227a556ff2377";
    hash = "sha256-S4lI6nJfg/iqHfa6l34da1Av4L8sDwlWfg2Vd4vh2A0=";
  };
  patches = [ ./fbdoom.patch ];
  postUnpack = ''
    echo $sourceRoot
    pwd
    ls $sourceRoot
    export sourceRoot=$sourceRoot/fbdoom
    export CROSS_COMPILE=riscv32-unknown-linux-uclibc-
    export NOSDL=1
  '';
  postPatch = ''
    sed -i 's/input_tty.o/input_evdev.o i_alsa_sound.o/' Makefile
    cp -v ${./fbdoom-overlay}/* .
  '';
  buildInputs = [ cnfa ];
  enableParallelBuilding = true;
  installPhase = ''
    ls -ltrh
    mkdir -p $out/bin/
    cp fbdoom $out/bin/
  '';
}
