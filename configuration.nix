{ pkgs, lib, ... }:

let
  test = pkgs.writeScript "dotest" ''
    #!/bin/sh
    echo $0
    #ping -c 4 -s 8000 -p 0x12 nas.localnet
    ls -l
    cat /proc/cpuinfo
    cat /proc/iomem
    #poweroff
  '';
  foo = pkgs.runCommandCC "foo" {
    passAsFile = [ "fork" "clone" ];
    fork = ''
      #include <stdio.h>
      #include <unistd.h>

      int main(int argc, char **argv) {
        int ret = fork();
        printf("fork == %d\n", ret);
      }
    '';
    clone = ''
      #define _GNU_SOURCE
      #include <stdio.h>
      #include <sched.h>
      #include <sys/wait.h>

      int main(int argc, char **argv) {
        int ret = clone(0, 0, SIGCHLD, 0);
        if (ret == -1) {
          perror("cant clone");
        } else {
          printf("clone == %d\n", ret);
        }
      }
    '';
  } ''
    mkdir -pv $out/bin/
    cp $forkPath fork.c
    cp $clonePath clone.c
    ${lib.optionalString (pkgs.stdenv.hostPlatform.libc == "musl") "$CC fork.c -o $out/bin/fork"}
    $CC clone.c -o $out/bin/clone

    fixupPhase
  '';
in
{
  imports = [
  ];
  initrd.packages = [
    (pkgs.callPackage ./evtest.nix {})
    (pkgs.callPackage ./fbtest {})
    foo
    #pkgs.evtest
    #pkgs.nix
  ];
  nixpkgs.overlays = [
    (self: super: {
      ubootTools = null;
    })
    (import ./overlay.nix)
  ];
  initrd.inittab = "ttyAMA0::once:${test}";
  kernel = {
    gzip_initrd = false;
    fb_console = false;
  };
  initrd.postInit = ''
  '';
}
