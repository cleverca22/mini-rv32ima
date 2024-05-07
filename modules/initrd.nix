{ lib, pkgs, config, ... }:
with lib;

let
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
        cp $ldso $out/lib
        chmod +w $out/lib/$(basename $ldso)
        chmod +w $x
        patchelf --set-interpreter $out/lib/$(basename $ldso) $x
      fi
    done
  '';
  # from https://github.com/celun/celun/blob/f4e681b896aae165506b7963eb6ac6d6c032145f/pkgs/mkCpio/default.nix
  borrowedMkCpio = (import pkgs.path { system = "x86_64-linux"; }).linux.overrideAttrs (attrs: {
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

      #mknod /dev/fb0 c 29 0
      #mknod /dev/vda b 254 0
      #mknod /dev/urandom c 1 9
      #mknod /dev/tty1 c 4 1
      #mknod /dev/null c 1 3
      #mknod /dev/input/event0 c 13 64
      #/bin/sh < /dev/tty1 > /dev/tty1 2>/dev/tty1 &

      echo boop
      #exit 42
      echo hello world
      mkdir /mnt
      # mount -t ext4 /dev/vda /mnt
      #exec /bin/sh
      getty -n -l /bin/sh 9600 /dev/ttyAMA0 &
      #getty -n -l /bin/sh 9600 /dev/tty1
      fbdoom -mb 2 -iwad /doom2.wad # -playdemo demo.lmp
    '';
    destination = "/init";
  };
  myenv = pkgs.buildEnv {
    name = "env";
    paths = with pkgs; [
      myinit
      shrunkenBinaries
    ];
  };
  myinitrd = pkgs.makeInitrd {
    prepend = [ console ];
    compressor = "cat";
    contents = [
      {
        object = "${myenv}/bin";
        symlink = "/bin";
      }
      {
        object = "${myenv}/init";
        symlink = "/init";
      }
      {
        object = ../DOOM2.WAD;
        symlink = "/doom2.wad";
      }
      {
        object = ../CS01-009.LMP;
        symlink = "/demo.lmp";
      }
    ];
  };
in
{
  options = {
    initrd.packages = mkOption {
      type = types.listOf types.package;
      default = [];
    };
  };
  config = {
    system.build.initrd = myinitrd;
  };
}
