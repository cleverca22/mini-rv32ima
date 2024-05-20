#include <stddef.h>
#include <string.h>

#include "i_sound.h"
#include "deh_str.h"

static snddevice_t sound_alsa_devices [] = {
  SNDDEVICE_SB,
};

static boolean use_sfx_prefix;

static void GetSfxLumpName(sfxinfo_t *sfx, char *buf, size_t buf_len) {
    // Linked sfx lumps? Get the lump number for the sound linked to.

    if (sfx->link != NULL)
    {
        sfx = sfx->link;
    }

    // Doom adds a DS* prefix to sound lumps; Heretic and Hexen don't
    // do this.

    if (use_sfx_prefix)
    {
        M_snprintf(buf, buf_len, "ds%s", DEH_String(sfx->name));
    }
    else
    {
        M_StringCopy(buf, DEH_String(sfx->name), buf_len);
    }
}

static boolean CacheSFX(sfxinfo_t *sfxinfo) {
  const int lumpnum = sfxinfo->lumpnum;
}

static boolean alsa_init_sound(boolean _use_sfx_prefix) {
  use_sfx_prefix = _use_sfx_prefix;
  puts(__func__);
  return true;
}

static void cnfa_shutdown(void) {
}

static int cnfa_get_sfx_lump_num(sfxinfo_t *sfx) {
  char namebuf[9];
  printf("cnfa_get_sfx_lump_num(%p) %s\n", sfx, sfx->name);

  GetSfxLumpName(sfx, namebuf, sizeof(namebuf));

  return W_GetNumForName(namebuf);
}

static void cnfa_update(void) {
  //puts(__func__);
}

static void cnfa_update_sound_params(int handle, int vol, int sep) {
  //puts(__func__);
}

static void print_sfxinfo(sfxinfo_t *sfxinfo) {
  printf("  tagname: %s\n", sfxinfo->tagname);
  printf("  name: %s\n", sfxinfo->name);
}

static int cnfa_start_sound(sfxinfo_t *sfxinfo, int channel, int vol, int sep) {
  if (strcmp(sfxinfo->name, "firsht") != 0) return channel;
  printf("cnfa_start_sound(%p, %d, %d, %d)\n", sfxinfo, channel, vol, sep);
  print_sfxinfo(sfxinfo);

  return channel;
}

static void cnfa_stop_sound(int handle) {
  //puts(__func__);
}

static boolean cnfa_sound_is_playing(int handle) {
  //puts(__func__);
}

static void cnfa_cache_sounds(sfxinfo_t *sounds, int num_sounds) {
  printf("cnfa_cache_sounds(%p, %d)\n", sounds, num_sounds);
  char namebuf[9];
  for (int i=0; i<num_sounds; ++i) {
    GetSfxLumpName(&sounds[i], namebuf, sizeof(namebuf));
    sounds[i].lumpnum = W_CheckNumForName(namebuf);
    if (sounds[i].lumpnum != -1) {
      CacheSFX(&sounds[i]);
    }
  }
}

sound_module_t sound_alsa_module = {
  .sound_devices = sound_alsa_devices,
  .num_sound_devices = arrlen(sound_alsa_devices),
  .Init = alsa_init_sound,
  .Shutdown = cnfa_shutdown,
  .GetSfxLumpNum = cnfa_get_sfx_lump_num,
  .Update = cnfa_update,
  .UpdateSoundParams = cnfa_update_sound_params,
  .StartSound = cnfa_start_sound,
  .StopSound = cnfa_stop_sound,
  .SoundIsPlaying = cnfa_sound_is_playing,
  .CacheSounds = cnfa_cache_sounds,
};
