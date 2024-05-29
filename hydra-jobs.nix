{ self, system, pkgs }:

let
  mkImage = pkgs.callPackage ./modules/mk-image.nix;
  minimal_cfg = {
    kernel = {
      fb_console = false;
      gfx = false;
      network = false;
      virtio = false;
    };
    initrd.shellOnTTY1 = false;
  };
  os-stuff = {
    fbdoom = mkImage { libc = "uclibc"; extraModules = [ ./configuration-fbdoom.nix ]; };
    base = mkImage { libc = "uclibc"; };
    tiny = mkImage { libc = "uclibc"; extraModules = [ minimal_cfg ]; };
    tiny-musl = mkImage { libc = "musl"; extraModules = [ minimal_cfg ]; };
  };
  doTest = let
    test = pkgs.writeScript "dotest" ''
      #!/bin/sh
      cat /proc/cpuinfo
      cat /proc/iomem
      poweroff
    '';
  in pkgs.runCommand "test" {
    buildInputs = [ pkgs.dos2unix ];
    image = mkImage [
      {
        initrd.inittab = "ttyAMA0::once:${test}";
      }
      minimal_cfg
    ] "uclibc";
  } ''
    mkdir -p $out/nix-support/
    cd $out
    ls $image/mini-rv32ima
    $image/mini-rv32ima/full-rv32ima.http -f $image/mini-rv32ima/Image -i $image/mini-rv32ima/initrd | tee logfile
    dos2unix logfile
    grep ': Kernel' logfile | awk '{ split($1, a, "-"); start=strtonum("0x"a[1]); end=strtonum("0x"a[2]); print $4 " " (end-start)/1024 " kb"; }' >> $out/nix-support/hydra-metrics
    for x in $image/mini-rv32ima/Image $image/mini-rv32ima/initrd; do
      echo $(basename $x) $(stat --printf=%s $x | awk '{ print $1/1024; }') kb >> $out/nix-support/hydra-metrics
    done
  '';
  test-stuff = {
    tests.simple = doTest;
  };
  packages = {
    inherit (self.packages.${system}) static-rv32ima windows-rv32ima cnfa qemu;
  };
in
  os-stuff // test-stuff // packages
