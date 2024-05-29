{
  inputs = {
    utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:cleverca22/nixpkgs/riscv-uclibc";
    #nixpkgs2.url = "path:/home/clever/apps/nixpkgs-master3";
  };
  outputs = { self, utils, nixpkgs }:
  let
    overlay = _: _: {
      inherit self;
    };
  in
  (utils.lib.eachSystem [ "x86_64-linux" "aarch64-linux" ] (system:
  let
    pkgs = import nixpkgs {
      inherit system; overlays = [ overlay (import ./overlay.nix) ];
      config.allowUnsupportedSystem = true;
    };
    mkDoTest = http: extra:
    let
      os = pkgs.callPackage ./os.nix { inherit nixpkgs; extraModules = extra; hostSystem = system; };
    in
      pkgs.writeShellScriptBin "dotest" ''
        ${pkgs.mini-rv32ima.override { inherit http; }}/bin/full-rv32ima -f ${os.toplevel}/Image -i ${os.toplevel}/initrd
      '';
    tests = {
      default = mkDoTest false [];
      http = mkDoTest true [];
      doom = mkDoTest false [ ./configuration-fbdoom.nix ];
      doom-http = mkDoTest true [ ./configuration-fbdoom.nix ];
      windows-test = let
        os = self.packets.${system}.os;
      in pkgs.writeShellScriptBin "windows-test" ''
        export PATH=$PATH:${pkgs.wine64}/bin/
        ls -lh ${self.packages.${system}.windows-rv32ima}/bin/
        wine64 ${self.packages.${system}.windows-rv32ima}/bin/full-rv32ima.exe -f ${os}/Image -i ${os}/initrd
      '';
    };
  in {
    packages = tests // (import ./packages.nix pkgs);
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
    hydraJobs = import ./hydra-jobs.nix { inherit self system pkgs; };
  })) // {
    nixConfig = {
      substituters = [ "https://hydra.angeldsis.com" "https://cache.nixos.org" ];
      trusted-public-keys = [ "hydra.angeldsis.com-1:7s6tP5et6L8Y6sX7XGIwzX5bnLp00MtUQ/1C9t1IBGE=" ];
    };
  };
}
