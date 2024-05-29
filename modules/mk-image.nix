{ self, libc, extraModules ? [], system, pkgs, zip, cpio }:
let
  toplevel = (pkgs.callPackage ../os.nix { inherit libc extraModules system; }).toplevel;
  output = pkgs.runCommand "build" {
    nativeBuildInputs = [ zip cpio ];
    passAsFile = [ "script" ];
    passthru = {
      inherit toplevel;
    };
    script = ''
      #!/bin/sh
      dir=$(dirname $0)
      exec "$dir/full-rv32ima" -f "$dir/Image" -i "$dir/initrd"
    '';
  } ''
    mkdir -p $out/mini-rv32ima $out/nix-support
    cd $out/mini-rv32ima
    cp ${toplevel}/* .
    cp ${self.packages.${system}.static-rv32ima}/bin/* .
    cp ${self.packages.${system}.static-http}/bin/full-rv32ima full-rv32ima.http
    cp $scriptPath launch
    chmod +x launch

    cat <<EOF > $out/nix-support/hydra-metrics
    initrd $(stat --printf=%s initrd) bytes
    Image $(stat --printf=%s Image) bytes
    EOF

    cd ..
    zip packed.zip mini-rv32ima/*
    echo "file binary-dist $out/packed.zip" >> $out/nix-support/hydra-build-products

    #mkdir unpacked
    #cd unpacked
    #cat ../mini-rv32ima/initrd | cpio -i
  '';
in
  output
