{
  inputs = {
    utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:cleverca22/nixpkgs/riscv-uclibc";
  };
  outputs = { self, utils, nixpkgs }:
  let
    overlay = self: super: {
      mini-rv32ima = self.callPackage ./mini-rv32ima {};
      myinit = self.writeTextFile {
        name = "init";
        executable = true;
        text = ''
          #!/bin/sh
          /bin/boop
          export PATH=/bin
          mknod /dev/console c 5 1
          echo boop > /dev/console
          #exit 42
          echo hello world
          exec /bin/sh > /dev/console < /dev/console
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
        $CC boop.c -o $out/bin/boop -fpie -pie
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
          self.boop
        ];
      };
      myinitrd = self.makeInitrd {
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
        cat ${self.myinitrd}/initrd | gunzip > $out
      '';
      rvkernel = self.pkgsCross.riscv32.linux_latest.override {
        enableCommonConfig = false;
        autoModules = false;
        structuredExtraConfig = with self.lib.kernel; {
          PAGE_OFFSET = freeform "0x80000000";
          PHYS_RAM_BASE = freeform "0x80000000";
          MMU = no;
          INITRAMFS_SOURCE = freeform "${self.myinitrdrename}";
          PHYS_RAM_BASE_FIXED = yes;
          SERIAL_OF_PLATFORM = yes;
          SERIAL_8250 = yes;
          SERIAL_EARLYCON = yes;
          NETFILTER = no;
          NONPORTABLE = yes;
          ARCH_RV32I = yes;
          SMP = no;
          RISCV_ISA_FALLBACK = no;
          CMODEL_MEDLOW = yes;
          FPU = no;
          RISCV_ISA_C = no; # mini-rv32ima lacks support for the c extensions
          BLOCK = no;
          USB_SUPPORT = no;
          MMC = no;
          FB = no;
          DRM = no;
          SND = no;
          NET = no;
          PCI = no;
          #MODULES = no; # TODO, breaks the nix build
          #CRYPTO = no;
          #LIBCRC32C = no;
          BPF_SYSCALL = no;
          CGROUPS = no;
          IKCONFIG = no;
          NAMESPACES = no;
          PROFILING = no;
          BINFMT_ELF_FDPIC = yes;
        };
      };
    };
  in
  utils.lib.eachSystem [ "x86_64-linux" "i686-linux" ] (system:
  let
    pkgs = import nixpkgs { inherit system; overlays = [ overlay ]; };
  in {
    packages = {
      inherit (pkgs) mini-rv32ima rvkernel myinitrd myenv;
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
