{ fetchFromGitHub, stdenv, lib, fetchurl, alsa-lib }:

let
  os_generic = fetchurl {
    url = "https://raw.githubusercontent.com/cntools/rawdraw/master/os_generic.h";
    hash = "sha256-99Itu+v0EHf86mIos+KXsBmOILKKDwbj37IyYEOwlfY=";
  };
  static = (stdenv.targetPlatform.isStatic || (stdenv.targetPlatform.config == "riscv32-unknown-linux-uclibc"));
in
stdenv.mkDerivation {
  name = "cnfa";
  src = fetchFromGitHub {
    owner = "cntools";
    repo = "cnfa";
    rev = "79515226faaf75b208266de664a430195809cdd6";
    hash = "sha256-L+MDz7x3lDDLdpMhMEqkQgqj7gItUUHz9LPT1xWrka4=";
  };
  buildInputs = [ alsa-lib ];
  patches = [ ./cnfa.patch ];
  preConfigure = ''
    cp ${os_generic} os_generic.h
  '';
  buildPhase = lib.optionalString static ''
    $CC -c CNFA.c -static -fpic -o CNFA.o -DCNFA_IMPLEMENTATION -lasound -lpthread
    $AR rcs libCNFA.a CNFA.o
  '';
  installPhase = if static then ''
    mkdir $out/{lib,include} -pv
    cp libCNFA.a $out/lib/
    cp CNFA.h $out/include/
  '' else ''
    mkdir $out/{lib,include} -pv
    cp libCNFA.so $out/lib/
    cp CNFA.h $out/include/
  '';
}
