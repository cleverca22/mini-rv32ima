{ lib, pkgs, config, ... }:
with lib;

let
  cfg = config.initrd;
  shrunkenBinaries = pkgs.runCommand "shrunken" {
    inputs = config.initrd.packages;
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
        cp -d $ldso $out/lib
        if [ -f $(dirname $ldso)/libc.so ]; then
          cp -v $(dirname $ldso)/libc.so $out/lib/
        fi
        if [ -f $(dirname $ldso)/ld-uClibc-1.0.47.so ]; then
          cp -v $(dirname $ldso)/ld-uClibc-1.0.47.so $out/lib/
        fi

        if [ -f $out/lib/$(basename $ldso) ]; then
          chmod +w $out/lib/$(basename $ldso)
        fi
        chmod +w $x
        patchelf --set-interpreter $out/lib/$(basename $ldso) $x
      fi
    done
    ls -lh $out/lib
  '';
  # from https://github.com/celun/celun/blob/f4e681b896aae165506b7963eb6ac6d6c032145f/pkgs/mkCpio/default.nix
  borrowedMkCpio = (import pkgs.path { system = pkgs.stdenv.buildPlatform.system; }).linux.overrideAttrs (attrs: {
    uts = [ "out" ];
    buildPhase = "make -C source/usr gen_init_cpio";
    installPhase = ''
      mkdir -p $out/usr $out/bin
      cp -rvt $out/usr source/usr/{gen_init_cpio,gen_initramfs.sh,default_cpio_list}
      cp -v source/usr/gen_init_cpio $out/bin
    '';
  });
  console = pkgs.runCommand "console.cpio" { nativeBuildInputs = [ borrowedMkCpio ]; } ''
    gen_init_cpio - > $out <<EOF
    dir /dev 0755 0 0
    nod /dev/console 0600 0 0 c 5 1
    EOF
  '';
  myinit = pkgs.writeTextFile {
    name = "init";
    executable = true;
    text = ''
      #!/bin/sh
      export PATH=/bin
      cd /
      mount -t proc proc proc
      mount -t sysfs sys sys
      mount -t debugfs debugfs sys/kernel/debug/
      mount -t devtmpfs devfs dev/

      mkdir /mnt
      #getty -n -l /bin/sh 9600 /dev/ttyAMA0 &
      ${lib.optionalString config.kernel.network "grep nameserver /proc/net/pnp > /etc/resolv.conf"}
      ${config.initrd.postInit}

      exec init
    '';
    destination = "/init";
  };
  inittab = pkgs.writeTextFile {
    name = "inittab";
    text = ''
      console::sysinit:echo "Welcome to mini-rv32ima Linux"
      ${lib.optionalString cfg.shellOnTTY1 "console::respawn:getty -n -l /bin/sh 9600 /dev/tty1"}
      ttyAMA0::respawn:getty -n -l /bin/sh 9600 /dev/ttyAMA0
      ${cfg.inittab}
    '';
    destination = "/etc/inittab";
  };
  myenv = pkgs.buildEnv {
    name = "myenv";
    paths = with pkgs; [
      myinit
      shrunkenBinaries
      inittab
    ];
  };
  myinitrd = pkgs.makeInitrd {
    prepend = [ console ];
    compressor = if config.kernel.gzip_initrd then "gzip -9n" else "cat";
    contents = [
      {
        object = "${myenv}/bin";
        symlink = "/bin";
      }
      {
        object = "${myenv}/etc";
        symlink = "/etc";
      }
      {
        object = "${myenv}/init";
        symlink = "/init";
      }
    ];
  };
in
{
  options = {
    initrd = {
      packages = mkOption {
        type = types.listOf types.package;
        default = [];
      };
      postInit = mkOption {
        type = types.lines;
        default = "";
      };
      shellOnTTY1 = mkOption {
        type = types.bool;
        default = true;
      };
      inittab = mkOption {
        type = types.lines;
        default = "";
      };
    };
  };
  config = {
    system.build.initrd = myinitrd;
    kernel.gfx = lib.mkIf config.initrd.shellOnTTY1 true;
  };
}
