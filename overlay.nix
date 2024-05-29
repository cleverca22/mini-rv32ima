self: super: {
  mini-rv32ima = self.callPackage ./mini-rv32ima {};
  cnfa = self.callPackage ./cnfa.nix {};
  qemu = super.qemu.overrideDerivation (old: { patches = old.patches ++ [ ./qemu.patch ]; });
  dtc = super.dtc.overrideAttrs (old: {
    patches = [ ./dtc-static.patch ];
    doCheck = false;
    src = super.fetchgit {
      url = "git://git.kernel.org/pub/scm/utils/dtc/dtc.git";
      rev = "ae26223a056e040b2d812202283d47c6e034d063";
      hash = "sha256-D4hUu3DHk0CCpwDDf1wjn1CQXRlc4vhqZrqAOkLTvBU=";
    };
  });
}
