{
  description = "Custom LCD firmware for the Zoom65 v3 screen module";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      devShells.${system}.default = pkgs.mkShell {
        packages = [
          pkgs.gcc-arm-embedded
          pkgs.gcc
          pkgs.gnumake
          (pkgs.python3.withPackages (p: [ p.pillow ]))
        ];
      };
    };
}
