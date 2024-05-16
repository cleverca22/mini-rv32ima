{ fetchFromGitHub, stdenv, lib, pkgconf, fetchurl, alsa-lib }:

let
  os_generic = fetchurl {
    url = "https://raw.githubusercontent.com/cntools/rawdraw/master/os_generic.h";
    hash = "sha256-99Itu+v0EHf86mIos+KXsBmOILKKDwbj37IyYEOwlfY=";
  };
in
stdenv.mkDerivation {
  name = "cnfa";
  src = fetchFromGitHub {
    owner = "cntools";
    repo = "cnfa";
    rev = "79515226faaf75b208266de664a430195809cdd6";
    hash = "sha256-L+MDz7x3lDDLdpMhMEqkQgqj7gItUUHz9LPT1xWrka4=";
  };
  buildInputs = [ pkgconf alsa-lib ];
  preConfigure = ''
    cp ${os_generic} os_generic.h
  '';
  installPhase = ''
    mkdir $out/{lib,include} -pv
    cp libCNFA.so $out/lib/
    cp CNFA.h $out/include/
  '';
}
