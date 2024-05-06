{
  inputs = {
    utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:cleverca22/nixpkgs/riscv-uclibc";
  };
  outputs = { self, utils, nixpkgs }:
  let
    overlay = self: super: let
      lib = self.lib;
    in {
      mini-rv32ima = self.callPackage ./mini-rv32ima {};
      boop = self.pkgsCross.riscv32-nommu.runCommandCC "boop" {
        passAsFile = [ "src" ];
        src = ''
          #include <sys/types.h>
          #include <sys/stat.h>
          #include <fcntl.h>
          #include <unistd.h>
          #include <sys/sysmacros.h>

          int main(int argc, char **argv) {
            mknod("/dev/console", 0600 | S_IFCHR, makedev(5, 1));
            int fd = open("/dev/console", O_RDWR);
            write(fd, "boop!\n", 6);
            return 43;
          }
        '';
      } ''
        mkdir -p $out/bin
        cat $srcPath > boop.c
        $CC boop.c -o $out/bin/boop -fpie -pie -s -Os
        file $out/bin/boop
      '';
    };
  in
  utils.lib.eachSystem [ "x86_64-linux" "i686-linux" ] (system:
  let
    pkgs = import nixpkgs { inherit system; overlays = [ overlay ]; };
  in {
    packages = rec {
      inherit (pkgs) mini-rv32ima rvkernel myinitrd myenv boop myinitrdrename borrowedMkCpio console shrunkenBinaries rvkernelWithoutInitrd;
      cfg = pkgs.rvkernel.configfile;
      default = pkgs.writeShellScriptBin "dotest" ''
        ${pkgs.mini-rv32ima}/bin/mini-rv32ima -f ${pkgs.rvkernel}/Image "$@"
      '';
      quicktest = pkgs.writeShellScriptBin "dotest" ''
        ${pkgs.mini-rv32ima}/bin/full-rv32ima -f ${os.kernel}/Image -i ${os.initrd}/initrd
      '';
      os = pkgs.callPackage ./os.nix { inherit nixpkgs; };
    };
    devShells = {
      kernel = pkgs.pkgsCross.riscv32-nommu.linux.overrideDerivation (drv: {
        nativeBuildInputs = drv.nativeBuildInputs ++ (with pkgs; [ ncurses pkg-config ]);
        makeFlags = drv.makeFlags ++ [ "O=rv32" ];
        shellHook = ''
          addToSearchPath PKG_CONFIG_PATH ${pkgs.ncurses.dev}/lib/pkgconfig
          echo to configure: 'make $makeFlags menuconfig'
          echo to build: 'time make $makeFlags zImage -j8'
        '';
      });
    };
    hydraJobs = {
      inherit (self.${system}.packages.os) initrd;
    };
  });
}
