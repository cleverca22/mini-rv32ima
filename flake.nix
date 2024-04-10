{
  inputs = {
    utils.url = "github:numtide/flake-utils";
  };
  outputs = { self, utils, nixpkgs }:
  let
    overlay = self: super: {
      mini-rv32ima = self.callPackage ./mini-rv32ima {};
      rvkernel = self.pkgsCross.riscv32.linux_latest.override {
        enableCommonConfig = false;
        autoModules = false;
        structuredExtraConfig = with self.lib.kernel; {
          PAGE_OFFSET = freeform "0x80000000";
          PHYS_RAM_BASE = freeform "0x80000000";
          MMU = no;
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
        };
      };
    };
  in
  utils.lib.eachSystem [ "x86_64-linux" "i686-linux" ] (system:
  let
    pkgs = import nixpkgs { inherit system; overlays = [ overlay ]; };
  in {
    packages = {
      inherit (pkgs) mini-rv32ima rvkernel;
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
