{ config, lib, pkgs, ... }:

with lib;

let
  isConfig = x:
    builtins.isAttrs x || builtins.isFunction x;

  optCall = f: x:
    if builtins.isFunction f
    then f x
    else f;

  mergeConfig = lhs_: rhs_:
    let
      lhs = optCall lhs_ { inherit pkgs; };
      rhs = optCall rhs_ { inherit pkgs; };
    in
    lhs // rhs //
    optionalAttrs (lhs ? packageOverrides) {
      packageOverrides = pkgs:
        optCall lhs.packageOverrides pkgs //
        optCall (attrByPath ["packageOverrides"] ({}) rhs) pkgs;
    };

  configType = mkOptionType {
    name = "nixpkgs config";
    check = x:
      let traceXIfNot = c:
        if c x then true
        else lib.traceSeqN 1 x false;
      in traceXIfNot isConfig;
    merge = args: fold (def: mergeConfig def.value) {};
  };

in

{
  options = {
    nixpkgs.overlays = mkOption {
      default = [];
    };

    nixpkgs.config = mkOption {
      default = {};
      example = literalExample
        ''
          { firefox.enableGeckoMediaPlayer = true;
            packageOverrides = pkgs: {
              firefox60Pkgs = pkgs.firefox60Pkgs.override {
                enableOfficialBranding = true;
              };
            };
          }
        '';
      type = configType;
      description = ''
        The configuration of the Nix Packages collection.  (For
        details, see the Nixpkgs documentation.)  It allows you to set
        package configuration options, and to override packages
        globally through the <varname>packageOverrides</varname>
        option.  The latter is a function that takes as an argument
        the <emphasis>original</emphasis> Nixpkgs, and must evaluate
        to a set of new or overridden packages.
      '';
    };
    nixpkgs.crossSystem = mkOption {
      internal = true;
      default = null;
    };

    nixpkgs.system = mkOption {
      type = types.str;
      example = "i686-linux";
      description = ''
        Specifies the Nix platform type for which NixOS should be built.
        If unset, it defaults to the platform type of your host system.
        Specifying this option is useful when doing distributed
        multi-platform deployment, or when building virtual machines.
      '';
    };

    nixpkgs.pkgs = mkOption {
    };

  };

  config = {
    _module.args.pkgs = config.nixpkgs.pkgs;
  };
}
