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
      cnfa = self.callPackage ./cnfa.nix {};
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
  (utils.lib.eachSystem [ "x86_64-linux" "aarch64-linux" ] (system:
  let
    pkgs = import nixpkgs {
      inherit system; overlays = [ overlay ];
      config.allowUnsupportedSystem = true;
    };
    mkDoTest = http: extra:
    let
      os = pkgs.callPackage ./os.nix { inherit nixpkgs; extraModules = extra; hostSystem = system; };
    in
      pkgs.writeShellScriptBin "dotest" ''
        ${pkgs.mini-rv32ima.override { inherit http; }}/bin/full-rv32ima -f ${os.toplevel}/Image -i ${os.toplevel}/initrd
      '';
  in {
    packages = rec {
      inherit (pkgs) mini-rv32ima cnfa;
      static-rv32ima = pkgs.pkgsStatic.mini-rv32ima;
      windows-rv32ima = pkgs.pkgsCross.mingwW64.mini-rv32ima;
      default = mkDoTest false [];
      http = mkDoTest true [];
      static-http = pkgs.pkgsStatic.mini-rv32ima.override { http=true; };
      os = (pkgs.callPackage ./os.nix { inherit nixpkgs; hostSystem = system; }).toplevel;
      doom = mkDoTest false [ ./configuration-fbdoom.nix ];
      doom-http = mkDoTest true [ ./configuration-fbdoom.nix ];
      windows-test = pkgs.writeShellScriptBin "windows-test" ''
        export PATH=$PATH:${pkgs.wine64}/bin/
        ls -lh ${windows-rv32ima}/bin/
        wine64 ${windows-rv32ima}/bin/full-rv32ima.exe -f ${os}/Image -i ${os}/initrd
      '';
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
          nativeBuildInputs = [ pkgs.zip pkgs.cpio ];
          passAsFile = [ "script" ];
          script = ''
            #!/bin/sh
            dir=$(dirname $0)
            exec "$dir/full-rv32ima" -f "$dir/Image" -i "$dir/initrd"
          '';
        } ''
          mkdir -p $out/mini-rv32ima $out/nix-support
          cd $out/mini-rv32ima
          cp ${toplevel}/* .
          cp ${self.packages.${system}.static-rv32ima}/bin/* .
          cp ${self.packages.${system}.static-http}/bin/full-rv32ima full-rv32ima.http
          cp $scriptPath launch
          chmod +x launch

          cat <<EOF > $out/nix-support/hydra-metrics
          initrd $(stat --printf=%s initrd) bytes
          Image $(stat --printf=%s Image) bytes
          EOF

          cd ..
          zip packed.zip mini-rv32ima/*
          echo "file binary-dist $out/packed.zip" >> $out/nix-support/hydra-build-products

          #mkdir unpacked
          #cd unpacked
          #cat ../mini-rv32ima/initrd | cpio -i
        '';
      in output;
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
            kernel.fb_console = false;
          }
        ];
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
    in {
      fbdoom = mkImage [ ./configuration-fbdoom.nix ];
      base = mkImage [ ];
      static-rv32ima = self.packages.${system}.static-rv32ima;
      windows-rv32ima = self.packages.${system}.windows-rv32ima;
      cnfa = self.packages.${system}.cnfa;
      tests.simple = doTest;
    };
  })) // {
    nixConfig = {
      substituters = [ "https://hydra.angeldsis.com" "https://cache.nixos.org" ];
      trusted-public-keys = [ "hydra.angeldsis.com-1:7s6tP5et6L8Y6sX7XGIwzX5bnLp00MtUQ/1C9t1IBGE=" ];
    };
  };
}
