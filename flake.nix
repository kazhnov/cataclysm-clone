{
  description = "C Development Environment";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux"; # Adjust for your architecture (e.g., aarch64-linux)
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        nativeBuildInputs = with pkgs; [
          gcc
          gnumake
          pkg-config
          clang-tools # Includes clangd for IDE LSP support
        ];
        
        buildInputs = with pkgs; [
          notcurses
        ];
      };
    };
}
