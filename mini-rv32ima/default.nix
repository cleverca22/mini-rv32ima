{ stdenv, dtc, xorg, mesa }:

stdenv.mkDerivation {
  src = ./.;
  name = "mini-rv32ima";
  makeFlags = "mini-rv32ima fat-rv32ima";
  installPhase = ''
    mkdir -p $out/bin
    cp mini-rv32ima{,.tiny} fat-rv32ima $out/bin/
  '';
  # dontStrip = true;
  buildInputs = [ dtc xorg.libX11 mesa ];
}
