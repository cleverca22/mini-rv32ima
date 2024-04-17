{
  inputs = {
    utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:cleverca22/nixpkgs/riscv-uclibc";
  };
  outputs = { self, utils, nixpkgs }:
  let
    overlay = self: super: {
      mini-rv32ima = self.callPackage ./mini-rv32ima {};
      # from https://github.com/celun/celun/blob/f4e681b896aae165506b7963eb6ac6d6c032145f/pkgs/mkCpio/default.nix
      borrowedMkCpio = self.linux.overrideAttrs (attrs: {
        uts = [ "out" ];
        buildPhase = "make -C source/usr gen_init_cpio";
        installPhase = ''
          mkdir -p $out/usr $out/bin
          cp -rvt $out/usr source/usr/{gen_init_cpio,gen_initramfs.sh,default_cpio_list}
          cp -v source/usr/gen_init_cpio $out/bin
        '';
      });
      console = self.runCommand "console.cpio" { buildInputs = [ self.borrowedMkCpio ]; } ''
        gen_init_cpio - > $out <<EOF
        dir /dev 0755 0 0
        nod /dev/console 0600 0 0 c 5 1
        EOF
      '';
      myinit = self.writeTextFile {
        name = "init";
        executable = true;
        text = ''
          #!/bin/sh
          export PATH=/bin
          cd /
          mount -t proc proc proc
          echo boop
          #exit 42
          echo hello world
          exec /bin/sh
        '';
        destination = "/init";
      };
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
      myenv = self.buildEnv {
        name = "env";
        paths = with self; [
          (self.pkgsCross.riscv32-nommu.busybox.override {
            enableMinimal = false;
            extraConfig = ''
              CONFIG_PING n
              CONFIG_PING6 n
              CONFIG_TRACEROUTE n
              CONFIG_TRACEROUTE6 n
              CONFIG_UDHCPC n
              CONFIG_UDHCPD n
              CONFIG_DHCPRELAY n
              CONFIG_DUMPLEASES n
              CONFIG_UDHCPC6 n
              CONFIG_MKTEMP n
            '';
          })
          self.myinit
          #self.boop
        ];
      };
      myinitrd = self.makeInitrd {
        prepend = [ self.console ];
        compressor = "cat";
        contents = [
          {
            object = "${self.myenv}/bin";
            symlink = "/bin";
          }
          {
            object = "${self.myenv}/init";
            symlink = "/init";
          }
        ];
      };
      myinitrdrename = self.runCommand "initrd.cpio" {} ''
        cat ${self.myinitrd}/initrd > $out
      '';
      rvkernel = self.pkgsCross.riscv32.linux_latest.override {
        enableCommonConfig = false;
        autoModules = false;
        structuredExtraConfig = with self.lib.kernel; {
          #CRYPTO = no;
          #LIBCRC32C = no;
          #MODULES = no; # TODO, breaks the nix build
          ARCH_RV32I = yes;
          BINFMT_ELF_FDPIC = yes;
          BLOCK = no;
          BPF_SYSCALL = no;
          CGROUPS = no;
          CMODEL_MEDLOW = yes;
          DRM = no;
          FB = no;
          FPU = no;
          IKCONFIG = no;
          INITRAMFS_SOURCE = freeform "${self.myinitrdrename}";
          MMC = no;
          MMU = no;
          NAMESPACES = no;
          NET = no;
          NETFILTER = no;
          NONPORTABLE = yes;
          PAGE_OFFSET = freeform "0x80000000";
          PCI = no;
          PHYS_RAM_BASE = freeform "0x80000000";
          PHYS_RAM_BASE_FIXED = yes;
          PROFILING = no;
          RISCV_ISA_C = no; # mini-rv32ima lacks support for the c extensions
          RISCV_ISA_FALLBACK = no;
          SERIAL_8250 = yes;
          SERIAL_EARLYCON = yes;
          SERIAL_OF_PLATFORM = yes;
          SMP = no;
          SND = no;
          USB_SUPPORT = no;
          SPI = no;
          MTD = no;
          SOUND = no;
          HID_SUPPORT = no;
        };
      };
    };
  in
  utils.lib.eachSystem [ "x86_64-linux" "i686-linux" ] (system:
  let
    pkgs = import nixpkgs { inherit system; overlays = [ overlay ]; };
  in {
    packages = {
      inherit (pkgs) mini-rv32ima rvkernel myinitrd myenv boop myinitrdrename borrowedMkCpio console;
      cfg = pkgs.rvkernel.configfile;
      default = pkgs.writeShellScriptBin "dotest" ''
        ${pkgs.mini-rv32ima}/bin/mini-rv32ima -f ${pkgs.rvkernel}/Image "$@"
      '';
    };
    devShells = {
      kernel = pkgs.rvkernel.overrideDerivation (drv: {
        nativeBuildInputs = drv.nativeBuildInputs ++ (with pkgs; [ ncurses pkg-config ]);
        makeFlags = drv.makeFlags ++ [ "O=rv32" ];
        shellHook = ''
          addToSearchPath PKG_CONFIG_PATH ${pkgs.ncurses.dev}/lib/pkgconfig
          echo to configure: 'make $makeFlags menuconfig'
          echo to build: 'time make $makeFlags zImage -j8'
        '';
      });
    };
  });
}
