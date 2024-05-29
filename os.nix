{ lib
, configuration ? ./configuration.nix
, extraModules ? []
, nixpkgs
, hostSystem
, libc ? "uclibc"
}:

let
  libcTable = {
    uclibc = lib.systems.examples.riscv32-nommu;
    musl = lib.systems.examples.riscv32-nommu-musl;
  };
  crossSystem = libcTable.${libc};
  system = hostSystem;
  pkgsModule = {config, ... }: {
    _file = ./os.nix;
    key = ./os.nix;
    config = {
      nixpkgs.pkgs = (import nixpkgs {
        inherit system crossSystem;
        config = config.nixpkgs.config;
        overlays = config.nixpkgs.overlays;
      });
      #nixpkgs.localSystem = {
      #  inherit system;
      #} // lib.optionalAttrs (crossSystem != null) {
      #  inherit crossSystem;
      #};
    };
  };
  baseModules = [
    ./modules/busybox.nix
    ./nixpkgs.nix
    ./modules/base.nix
    ./modules/kernel.nix
    ./modules/toplevel.nix
    ./modules/initrd.nix
  ];
  other = {
    _module.check = true;
    _module.args = {};
  };
  evalConfig = modules: lib.evalModules {
    prefix = [];
    modules = modules ++ baseModules ++ [ pkgsModule other ] ++ extraModules;
  };
in
rec {
  test1 = evalConfig [ configuration ];
  busy = test1.config.system.build.busybox;
  kernel = test1.config.system.build.kernel;
  toplevel = test1.config.system.build.toplevel;
  initrd = test1.config.system.build.initrd;
}
