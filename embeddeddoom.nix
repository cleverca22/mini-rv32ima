{ path, runCommandCC, fetchFromGitHub, lib, gcc }:

let
  pkgs32 = import path { system = "i686-linux"; };
in
runCommandCC "emdoom" {
  src = fetchFromGitHub {
    owner = "cnlohr";
    repo = "embeddeddoom";
    rev = "68d3f729913185fda994bb9f09af66c02990a49e";
    hash = "sha256-IwjS8NRfzxH3CYYqX5jxXF5fT+hXvEP3XaaUZprhzjA=";
  };
  nativeBuildInputs = [ pkgs32.gcc ];
} ''
  # todo, replace i_video, I_FinishUpdate should blit to fb0
  unpackPhase
  cd $sourceRoot
  mkdir -p $out/bin
  cd src
  make support/rawwad_use.c
  $CC -DNORMALUNIX -DLINUX -DMAXPLAYERS=1 -DDISABLE_NETWORK i_main.c m_argv.c d_main.c g_game.c p_tick.c hu_stuff.c hu_lib.c r_draw.c v_video.c m_bbox.c i_system.c i_video_console.c d_net.c m_menu.c am_map.c p_setup.c w_wad.c -o $out/emdoom
''
