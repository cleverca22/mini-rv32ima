{ runCommandCC }:

runCommandCC "evtest" {
  src = ./evtest.c;
} ''
  mkdir -p $out/bin
  cp $src evtest.c
  $CC evtest.c -o $out/bin/evtest
''
