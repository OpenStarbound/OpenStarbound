{
  description = "OpenStarbound";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});
    in
    {
      packages = forAllSystems (pkgs: rec {
        openstarbound = pkgs.callPackage ./nix/package.nix {
          rev = self.shortRev or self.dirtyShortRev or "unknown";
        };
        default = openstarbound;
      });

      apps = forAllSystems (
        pkgs:
        let
          pkg = self.packages.${pkgs.stdenv.hostPlatform.system}.openstarbound;
        in
        rec {
          client = {
            type = "app";
            program = "${pkg}/bin/openstarbound";
          };
          server = {
            type = "app";
            program = "${pkg}/bin/openstarbound-server";
          };
          default = client;
        }
      );

      overlays.default = final: _: {
        openstarbound = final.callPackage ./nix/package.nix { };
      };

      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          inputsFrom = [ self.packages.${pkgs.stdenv.hostPlatform.system}.openstarbound ];
          packages = [ pkgs.gdb ];
        };
      });
    };
}
