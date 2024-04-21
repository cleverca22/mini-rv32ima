{
  inputs = {
    utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:cleverca22/nixpkgs/riscv-uclibc";
  };
  outputs = { self, utils, nixpkgs }:
  let
    overlay = self: super: let
      lib = self.lib;
      mkKernel = { initrd ? null }:
      self.pkgsCross.riscv32.linux_latest.override {
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
        } // lib.optionalAttrs (initrd != null) {
          INITRAMFS_SOURCE = freeform "${initrd}";
        };
      };
    in {
      rvkernel = mkKernel { initrd = self.myinitrdrename; };
      rvkernelWithoutInitrd = mkKernel {};
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
      shrunkenBinaries = self.runCommand "shrunken" {
        inputs = [
          (self.pkgsCross.riscv32-nommu.busybox.override {
            enableMinimal = true;
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
              CONFIG_HUSH y
              CONFIG_HUSH_EXPORT y
              CONFIG_MOUNT y
              CONFIG_ECHO y
              # CONFIG_SH_IS_HUSH y
              CONFIG_SH_IS_NONE n
              CONFIG_SHELL_HUSH y
              CONFIG_LS y
              CONFIG_DF y
              CONFIG_DU y
              CONFIG_ENV y
              CONFIG_SORT y
              CONFIG_HEAD y
              CONFIG_TAIL y
              CONFIG_WC y
              CONFIG_DMESG y
              CONFIG_REBOOT y
              CONFIG_POWEROFF y
              CONFIG_CAT y
              CONFIG_XXD y
              CONFIG_HUSH_INTERACTIVE y
            '';
              #${builtins.readFile ./configs/busybox_config}
          })
          #self.boop
        ];
      } ''
        mkdir -pv $out/bin $out/lib
        for x in $inputs; do
          echo now for $x
          cp -va $x/bin/* $out/bin/
        done
        for x in $out/bin/*; do
          if [[ -f $x && ! -L $x ]]; then
            echo its a file $x
            ldso=$(patchelf --print-interpreter $x)
            cp $ldso $out/lib
            chmod +w $x
            patchelf --set-interpreter $out/lib/$(basename $ldso) $x
          fi
        done
      '';
      myinit = self.writeTextFile {
        name = "init";
        executable = true;
        text = ''
          #!/bin/sh
          export PATH=/bin
          cd /
          mount -t proc proc proc
          mount -t sysfs sys sys
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
          self.myinit
          shrunkenBinaries
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
    };
  in
  utils.lib.eachSystem [ "x86_64-linux" "i686-linux" ] (system:
  let
    pkgs = import nixpkgs { inherit system; overlays = [ overlay ]; };
  in {
    packages = {
      inherit (pkgs) mini-rv32ima rvkernel myinitrd myenv boop myinitrdrename borrowedMkCpio console shrunkenBinaries rvkernelWithoutInitrd;
      cfg = pkgs.rvkernel.configfile;
      default = pkgs.writeShellScriptBin "dotest" ''
        ${pkgs.mini-rv32ima}/bin/mini-rv32ima -f ${pkgs.rvkernel}/Image "$@"
      '';
      quicktest = pkgs.writeShellScriptBin "dotest" ''
        ${pkgs.mini-rv32ima}/bin/mini-rv32ima.dtb -f ${pkgs.rvkernelWithoutInitrd}/Image -i ${pkgs.myinitrd}/initrd
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
