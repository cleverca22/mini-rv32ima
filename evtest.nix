{ runCommandCC }:

runCommandCC "evtest" {
  src = ./evtest.c;
  # hardeningDisable = [ "stackprotector" ];
} ''
  mkdir -p $out/bin
  cp $src evtest.c
  $CC evtest.c -o $out/bin/evtest2
  fixupPhase
''
