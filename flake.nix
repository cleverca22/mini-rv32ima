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
      dtc = super.dtc.overrideAttrs (old: {
        patches = [ ./dtc-static.patch ];
        doCheck = false;
        src = super.fetchgit {
          url = "git://git.kernel.org/pub/scm/utils/dtc/dtc.git";
          rev = "ae26223a056e040b2d812202283d47c6e034d063";
          hash = "sha256-D4hUu3DHk0CCpwDDf1wjn1CQXRlc4vhqZrqAOkLTvBU=";
        };
      });
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
  utils.lib.eachSystem [ "x86_64-linux" "aarch64-linux" ] (system:
  let
    pkgs = import nixpkgs { inherit system; overlays = [ overlay ]; };
    mkDoTest = extra:
    let
      os = pkgs.callPackage ./os.nix { inherit nixpkgs; extraModules = extra; hostSystem = system; };
    in
      pkgs.writeShellScriptBin "dotest" ''
        ${pkgs.mini-rv32ima}/bin/full-rv32ima -f ${os.toplevel}/Image -i ${os.toplevel}/initrd
      '';
  in {
    packages = rec {
      inherit (pkgs) mini-rv32ima;
      static-rv32ima = pkgs.pkgsStatic.mini-rv32ima;
      default = pkgs.writeShellScriptBin "dotest" ''
        ${pkgs.mini-rv32ima}/bin/full-rv32ima -f ${os}/Image -i ${os}/initrd
      '';
      os = (pkgs.callPackage ./os.nix { inherit nixpkgs; hostSystem = system; }).toplevel;
      doom = mkDoTest [ ./configuration-fbdoom.nix ];
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
    hydraJobs = let
      mkImage = extra:
      let
        toplevel = (pkgs.callPackage ./os.nix { inherit nixpkgs; extraModules = extra; hostSystem = system; }).toplevel;
        output = pkgs.runCommand "build" {
          nativeBuildInputs = [ pkgs.zip ];
          passAsFile = [ "script" ];
          script = ''
            #!/bin/sh
            dir=$(dirname $0)
            exec "$dir/full-rv32ima" -f "$dir/Image" -i "$dir/initrd"
          '';
        } ''
          mkdir -p $out/unpacked $out/nix-support
          cd $out/unpacked
          cp ${toplevel}/* .
          cp ${self.packages.${system}.static-rv32ima}/bin/* .
          cp $scriptPath launch
          chmod +x launch

          zip ../packed.zip *
          echo "file binary-dist $out/packed.zip" >> $out/nix-support/hydra-build-products

          cat <<EOF > $out/nix-support/hydra-metrics
          initrd $(stat --printf=%s initrd) bytes
          Image $(stat --printf=%s Image) bytes
          EOF
        '';
      in output;
    in {
      fbdoom = mkImage [ ./configuration-fbdoom.nix ];
      base = mkImage [ ];
      static-rv32ima = self.packages.${system}.static-rv32ima;
    };
  });
}
