{ stdenv, dtc }:

stdenv.mkDerivation {
  src = ./.;
  name = "mini-rv32ima";
  makeFlags = "mini-rv32ima mini-rv32ima.dtb";
  installPhase = ''
    mkdir -p $out/bin
    cp mini-rv32ima{,.tiny,.dtb} $out/bin/
  '';
  buildInputs = [ dtc ];
}
