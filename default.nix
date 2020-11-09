{ pkgs ? (import <nixpkgs> {}), ... }:
with pkgs;
stdenv.mkDerivation {
  pname = "snek";
  version = "0.0.1";

  src = ./.;

  nativeBuildInputs = [ meson ninja pkgconfig ];
  buildInputs = [ sfml ];
}
