{
  description = "A presentation about sandboxes";

  inputs = {
    nixpkgs.url = "nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    let
      mergeEnvs = pkgs: envs:
        pkgs.mkShell (builtins.foldl' (a: v: {
          buildInputs = a.buildInputs ++ v.buildInputs;
          nativeBuildInputs = a.nativeBuildInputs ++ v.nativeBuildInputs;
          propagatedBuildInputs = a.propagatedBuildInputs
            ++ v.propagatedBuildInputs;
          propagatedNativeBuildInputs = a.propagatedNativeBuildInputs
            ++ v.propagatedNativeBuildInputs;
          shellHook = a.shellHook + "\n" + v.shellHook;
        }) (pkgs.mkShell { }) envs);
    in flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        version = "0.1.0";
      in rec {
        packages = {
          application = 
            with pkgs;
            stdenv.mkDerivation {
              pname = "application";
              src = ./.;
              inherit version;

              nativeBuildInputs = with pkgs; [
                ninja
                cmake
                extra-cmake-modules
                libcap
                libseccomp
              ];
            };
          sandboxed = pkgs.writeShellScriptBin "sandboxed" ''
            LD_PRELOAD=${packages.application}/lib/libsandbox.so ${packages.application}/bin/application
          '';
        };
        defaultPackage = packages.application;

        devShells = {
          application = packages.application;
        };
        devShell = devShells.application;
      });
}
