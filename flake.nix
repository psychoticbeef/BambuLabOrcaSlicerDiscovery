{
  description = "SSDP responder for Bambu Lab printer discovery (OrcaSlicer / Bambu Studio)";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
  let
    forAllSystems = nixpkgs.lib.genAttrs [ "x86_64-linux" "aarch64-linux" ];
  in
  {
    packages = forAllSystems (system:
      let pkgs = nixpkgs.legacyPackages.${system}; in {
        default = pkgs.stdenv.mkDerivation {
          pname = "ssdp-printer-responder";
          version = "unstable-${self.shortRev or self.dirtyShortRev or "dev"}";

          src = self;

          buildInputs = [ pkgs.boost pkgs.yaml-cpp ];

          installPhase = ''
            mkdir -p $out/bin
            cp ssdp_printer $out/bin/
          '';
        };
      });

    devShells = forAllSystems (system:
      let pkgs = nixpkgs.legacyPackages.${system}; in {
        default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.default ];
        };
      });
  };
}
