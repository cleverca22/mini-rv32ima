{ runCommandCC }:

runCommandCC "fbtest" {} ''
  mkdir -pv $out/bin
  $CC ${./fbtest.c} -Wall -o $out/bin/fbtest
  fixupPhase
''
