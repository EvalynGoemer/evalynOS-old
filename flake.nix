{
    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
    };

    outputs =
        { self, nixpkgs }:
        let
            system = "x86_64-linux";
            pkgs = nixpkgs.legacyPackages.${system};
        in
        {
            devShells.${system}.default = pkgs.mkShell {
                nativeBuildInputs = with pkgs; [
                    # make bootstrap
                    wget
                    gcc
                    pkg-config
                    libarchive.dev
                    openssl.dev
                    zlib
                    zlib.dev

                    # everything else
                    clang
                    nasm
                    libisoburn
                    libllvm.lib
                    lld
                    qemu
                ];
            };
        };
}
