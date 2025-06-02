{unstable ? import <nixos-unstable> {},
pkgs ? import <nixpkgs> {},
arm ? import <nixpkgs> {
  crossSystem = {
    config = "arm-none-eabihf";
    abi = "armv7e-m";
    libc = "newlib";
    gcc = {
      float-abi = "hard";
      fpu = "fpv4-sp-d16";
      cpu = "cortex-m4";
    };
  };
}}:
let my-python-packages = ps: with ps; [
  pyserial
  pygdbmi
  (
    buildPythonPackage rec {
      pname = "kflash";
      version = "1.1.6";
      src = fetchPypi {
        inherit pname version;
        sha256 = "sha256-7F+7NeqBqRyJ9FjTdeJKbas9OGXGmn5iUFEbLpP0tpU=";
      };
      doCheck = false;
      propagatedBuildInputs = [
        # Specify dependencies
        pyserial pyelftools
      ];
    }
    )
  ];
in
  arm.callPackage (
    {mkShell, gcc-arm-embedded, gcc, gdb}:
    mkShell (rec {
      nativeBuildInputs = [
        pkgs.gnumake
        pkgs.wabt
        gdb
        gcc-arm-embedded
        pkgs.cmake
        (pkgs.python3.withPackages my-python-packages)
        pkgs.minicom
        pkgs.inetutils
        pkgs.binutils
        unstable.openocd
      ];
      buildInputs = [
        pkgs.systemd
        arm.newlib
        arm.stdenv.cc.cc.lib
        pkgs.wabt
      ];
      shellHook = "
      export ARMGCC=${arm.pkgsBuildTarget.gcc-arm-embedded}/bin
      ";
    })
    ) {}
