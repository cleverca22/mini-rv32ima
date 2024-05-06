{ stdenv, fetchFromGitHub, lib }:

stdenv.mkDerivation {
  name = "fbdoom";
  src = fetchFromGitHub {
    owner = "maximevince";
    repo = "fbDOOM";
    rev = "912c51c4d0e5aa899bb534e8c77227a556ff2377";
    hash = lib.fakeHash;
  };
}
