{
  nixConfig.bash-prompt = "[nix(develop):\\w]$ ";

  inputs = {
    nixos-unstable.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs, nixos-unstable }: {
    devShells.x86_64-linux.default =
      let pkgs = nixpkgs.legacyPackages.x86_64-linux;
      arm =  (import nixpkgs {
        localSystem = "x86_64-linux";
        crossSystem = {
          config = "arm-none-eabihf";
          abi = "armv7e-m";
          libc = "newlib";
          gcc = {
            float-abi = "hard";
            fpu = "fpv4-sp-d16";
            cpu = "cortex-m4";
          };
        };});
        unstable = nixos-unstable.legacyPackages.x86_64-linux;
      in import ./shell.nix { inherit pkgs; inherit arm; inherit unstable;};
    };
  }
