{ stdenv }:

stdenv.mkDerivation {
  src = ./.;
  name = "mini-rv32ima";
  makeFlags = "mini-rv32ima";
  installPhase = ''
    mkdir -p $out/bin
    cp mini-rv32ima{,.tiny} $out/bin/
  '';
}
