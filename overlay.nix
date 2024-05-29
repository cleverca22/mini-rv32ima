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
  openssl = super.openssl.overrideAttrs (old: {
    # openssl-3.0.13/crypto/threads_pthread.c:272:(.text+0x33c): undefined reference to `__atomic_load_8'
    NIX_LDFLAGS = [ "-latomic" ];
  });
  alsa-lib = super.alsa-lib.overrideAttrs (old: {
    configureFlags = [
      "--disable-mixer"
      "--enable-static"
      "--disable-shared"
      "--with-pcm-plugins=copy,linear"
      #"--help"
    ];
  });
}
