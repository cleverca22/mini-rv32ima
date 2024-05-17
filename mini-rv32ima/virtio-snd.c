#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <CNFA.h>

#include "virtio.h"
#include "plic.h"

enum {
  VIRTIO_SND_R_JACK_INFO = 1,
  VIRTIO_SND_R_PCM_INFO = 0x0100,
  VIRTIO_SND_R_CHMAP_INFO = 0x0200,
  VIRTIO_SND_S_OK = 0x8000,
};

enum {
  VIRTIO_SND_PCM_RATE_5512 = 0,
  VIRTIO_SND_PCM_RATE_8000,
  VIRTIO_SND_PCM_RATE_11025,
  VIRTIO_SND_PCM_RATE_16000,
  VIRTIO_SND_PCM_RATE_22050,
  VIRTIO_SND_PCM_RATE_32000,
  VIRTIO_SND_PCM_RATE_44100,
  VIRTIO_SND_PCM_RATE_48000,
  VIRTIO_SND_PCM_RATE_64000,
  VIRTIO_SND_PCM_RATE_88200,
  VIRTIO_SND_PCM_RATE_96000,
  VIRTIO_SND_PCM_RATE_176400,
  VIRTIO_SND_PCM_RATE_192000,
  VIRTIO_SND_PCM_RATE_384000
};


/* supported PCM sample formats */
enum {
  /* analog formats (width / physical width) */
  VIRTIO_SND_PCM_FMT_IMA_ADPCM = 0,   /*  4 /  4 bits */
  VIRTIO_SND_PCM_FMT_MU_LAW,          /*  8 /  8 bits */
  VIRTIO_SND_PCM_FMT_A_LAW,           /*  8 /  8 bits */
  VIRTIO_SND_PCM_FMT_S8,              /*  8 /  8 bits */
  VIRTIO_SND_PCM_FMT_U8,              /*  8 /  8 bits */
  VIRTIO_SND_PCM_FMT_S16,             /* 16 / 16 bits */
  VIRTIO_SND_PCM_FMT_U16,             /* 16 / 16 bits */
  VIRTIO_SND_PCM_FMT_S18_3,           /* 18 / 24 bits */
  VIRTIO_SND_PCM_FMT_U18_3,           /* 18 / 24 bits */
  VIRTIO_SND_PCM_FMT_S20_3,           /* 20 / 24 bits */
  VIRTIO_SND_PCM_FMT_U20_3,           /* 20 / 24 bits */
  VIRTIO_SND_PCM_FMT_S24_3,           /* 24 / 24 bits */
  VIRTIO_SND_PCM_FMT_U24_3,           /* 24 / 24 bits */
  VIRTIO_SND_PCM_FMT_S20,             /* 20 / 32 bits */
  VIRTIO_SND_PCM_FMT_U20,             /* 20 / 32 bits */
  VIRTIO_SND_PCM_FMT_S24,             /* 24 / 32 bits */
  VIRTIO_SND_PCM_FMT_U24,             /* 24 / 32 bits */
  VIRTIO_SND_PCM_FMT_S32,             /* 32 / 32 bits */
  VIRTIO_SND_PCM_FMT_U32,             /* 32 / 32 bits */
  VIRTIO_SND_PCM_FMT_FLOAT,           /* 32 / 32 bits */
  VIRTIO_SND_PCM_FMT_FLOAT64,         /* 64 / 64 bits */
  /* digital formats (width / physical width) */
  VIRTIO_SND_PCM_FMT_DSD_U8,          /*  8 /  8 bits */
  VIRTIO_SND_PCM_FMT_DSD_U16,         /* 16 / 16 bits */
  VIRTIO_SND_PCM_FMT_DSD_U32,         /* 32 / 32 bits */
  VIRTIO_SND_PCM_FMT_IEC958_SUBFRAME  /* 32 / 32 bits */
};

/* standard channel position definition */
enum {
    VIRTIO_SND_CHMAP_NONE = 0,  /* undefined */
    VIRTIO_SND_CHMAP_NA,        /* silent */
    VIRTIO_SND_CHMAP_MONO,      /* mono stream */
    VIRTIO_SND_CHMAP_FL,        /* front left */
    VIRTIO_SND_CHMAP_FR,        /* front right */
    VIRTIO_SND_CHMAP_RL,        /* rear left */
    VIRTIO_SND_CHMAP_RR,        /* rear right */
    VIRTIO_SND_CHMAP_FC,        /* front center */
    VIRTIO_SND_CHMAP_LFE,       /* low frequency (LFE) */
    VIRTIO_SND_CHMAP_SL,        /* side left */
    VIRTIO_SND_CHMAP_SR,        /* side right */
    VIRTIO_SND_CHMAP_RC,        /* rear center */
    VIRTIO_SND_CHMAP_FLC,       /* front left center */
    VIRTIO_SND_CHMAP_FRC,       /* front right center */
    VIRTIO_SND_CHMAP_RLC,       /* rear left center */
    VIRTIO_SND_CHMAP_RRC,       /* rear right center */
    VIRTIO_SND_CHMAP_FLW,       /* front left wide */
    VIRTIO_SND_CHMAP_FRW,       /* front right wide */
    VIRTIO_SND_CHMAP_FLH,       /* front left high */
    VIRTIO_SND_CHMAP_FCH,       /* front center high */
    VIRTIO_SND_CHMAP_FRH,       /* front right high */
    VIRTIO_SND_CHMAP_TC,        /* top center */
    VIRTIO_SND_CHMAP_TFL,       /* top front left */
    VIRTIO_SND_CHMAP_TFR,       /* top front right */
    VIRTIO_SND_CHMAP_TFC,       /* top front center */
    VIRTIO_SND_CHMAP_TRL,       /* top rear left */
    VIRTIO_SND_CHMAP_TRR,       /* top rear right */
    VIRTIO_SND_CHMAP_TRC,       /* top rear center */
    VIRTIO_SND_CHMAP_TFLC,      /* top front left center */
    VIRTIO_SND_CHMAP_TFRC,      /* top front right center */
    VIRTIO_SND_CHMAP_TSL,       /* top side left */
    VIRTIO_SND_CHMAP_TSR,       /* top side right */
    VIRTIO_SND_CHMAP_LLFE,      /* left LFE */
    VIRTIO_SND_CHMAP_RLFE,      /* right LFE */
    VIRTIO_SND_CHMAP_BC,        /* bottom center */
    VIRTIO_SND_CHMAP_BLC,       /* bottom left center */
    VIRTIO_SND_CHMAP_BRC        /* bottom right center */
};

enum {
  VIRTIO_SND_D_OUTPUT = 0,
  VIRTIO_SND_D_INPUT
};


typedef struct {
  uint32_t jacks;
  uint32_t streams;
  uint32_t chmaps;
} virtio_snd_config;

typedef struct {
  uint32_t code;
} virtio_snd_hdr;

typedef struct {
  uint32_t hda_fn_nid;
} virtio_snd_info;

typedef struct {
  virtio_snd_hdr hdr;
  uint32_t start_id;
  uint32_t count;
  uint32_t size; // size of a singly reply
} virtio_snd_query_info;

typedef struct {
  virtio_snd_info hdr;
  uint32_t features;
  uint32_t hda_reg_defconf;
  uint32_t hda_reg_caps;
  uint8_t connected;
  uint8_t padding[7];
} virtio_snd_jack_info;

typedef struct {
  virtio_snd_info hdr;
  uint32_t features;
  uint64_t formats;
  uint64_t rates;
  uint8_t direction;
  uint8_t channels_min;
  uint8_t channels_max;
  uint8_t padding[5];
} virtio_snd_pcm_info;

#define VIRTIO_SND_CHMAP_MAX_SIZE 18

typedef struct {
  virtio_snd_info hdr;
  uint8_t direction;
  uint8_t channels;
  uint8_t positions[VIRTIO_SND_CHMAP_MAX_SIZE];
} virtio_snd_chmap_info;

static virtio_snd_config cfg = {
  .jacks = 1,
  .streams = 1,
  .chmaps = 1,
};

static struct CNFADriver *audio_host = NULL;

static bool guest_playing = false;

static uint32_t virtio_snd_config_load(struct virtio_device *dev, uint32_t offset) {
  uint32_t ret = 0;
  if (offset < sizeof(cfg)) {
    uint32_t *cfg32 = (uint32_t*)&cfg;
    ret = cfg32[offset/4];
  }
  printf("virtio_snd_config_load(%p, %d) == 0x%x\n", dev, offset, ret);
  return ret;
}

static void virtio_snd_config_store(struct virtio_device *dev, uint32_t offset, uint32_t val) {
  printf("virtio_snd_config_store(%p, %d, %d)\n", dev, offset, val);
}

static void virtio_process_control(struct virtio_device *dev, struct virtio_desc_internal *chain, int chain_length, uint16_t start_idx) {
  assert((chain[0].flags & 2) == 0);
  assert(chain[0].message_len >= 4);
  virtio_snd_hdr *hdr = chain[0].message;
  virtio_snd_hdr *hdr2;
  virtio_snd_query_info *query;
  printf("control code %d\n", hdr->code);
  switch (hdr->code) {
  case VIRTIO_SND_R_JACK_INFO:
  {
    assert(chain[0].message_len == sizeof(*query));
    query = chain[0].message;
    printf("VIRTIO_SND_R_JACK_INFO start=%d count=%d size=%d\n", query->start_id, query->count, query->size);
    assert(query->size == sizeof(virtio_snd_jack_info));
    assert(chain[2].message_len == (sizeof(virtio_snd_jack_info) * query->count));
    virtio_snd_jack_info *info = chain[2].message;
    for (int i=0; i<query->count; i++) {
      int jack = i + query->start_id;
      info[i].hdr.hda_fn_nid = 0;
      info[i].features = 0;
      info[i].hda_reg_defconf = 0;
      info[i].hda_reg_caps = 0;
      info[i].connected = 1;
      memset(&info[i].padding, 0, sizeof(info[i].padding));
    }
    assert(chain[1].message_len == sizeof(*hdr2));
    hdr2 = chain[1].message;
    hdr2->code = VIRTIO_SND_S_OK;
    virtio_flag_completion(dev, 0, start_idx, chain[2].message_len);
    break;
  }
  case VIRTIO_SND_R_PCM_INFO:
  {
    assert(chain[0].message_len == sizeof(*query));
    query = chain[0].message;
    printf("VIRTIO_SND_R_PCM_INFO start=%d count=%d size=%d\n", query->start_id, query->count, query->size);
    assert(query->size == sizeof(virtio_snd_pcm_info));
    assert(chain[2].message_len == (sizeof(virtio_snd_pcm_info) * query->count));
    virtio_snd_pcm_info *info = chain[2].message;
    for (int i=0; i<query->count; i++) {
      info[0].features = 0;
      info[0].formats = (1<<VIRTIO_SND_PCM_FMT_S16);
      info[0].rates = (1<<VIRTIO_SND_PCM_RATE_44100);
      info[0].direction = VIRTIO_SND_D_OUTPUT;
      info[0].channels_min = 1;
      info[0].channels_max = 1;
      memset(&info[i].padding, 0, sizeof(info[i].padding));
    }
    assert(chain[1].message_len == sizeof(*hdr2));
    hdr2 = chain[1].message;
    hdr2->code = VIRTIO_SND_S_OK;
    virtio_flag_completion(dev, 0, start_idx, chain[2].message_len);
    break;
  }
  case VIRTIO_SND_R_CHMAP_INFO:
  {
    assert(chain[0].message_len == sizeof(*query));
    query = chain[0].message;
    printf("VIRTIO_SND_R_CHMAP_INFO start=%d count=%d size=%d\n", query->start_id, query->count, query->size);
    assert(query->size == sizeof(virtio_snd_chmap_info));
    assert(chain[2].message_len == (sizeof(virtio_snd_chmap_info) * query->count));
    virtio_snd_chmap_info *info = chain[2].message;
    for (int i=0; i<query->count; i++) {
      info[i].direction = VIRTIO_SND_D_OUTPUT;
      info[i].channels = 1;
      info[i].positions[0] = VIRTIO_SND_CHMAP_MONO;
    }
    assert(chain[1].message_len == sizeof(*hdr2));
    hdr2 = chain[1].message;
    hdr2->code = VIRTIO_SND_S_OK;
    virtio_flag_completion(dev, 0, start_idx, chain[2].message_len);
  }
  }
}

static void virtio_snd_process_command(struct virtio_device *dev, struct virtio_desc_internal *chain, int chain_length, int queue, uint16_t start_idx) {
  if (queue == 0) {
    printf("virtio_snd_process_command(%p, %p, length %d, queue %d, %d)\n", dev, chain, chain_length, queue, start_idx);
    for (int i=0; i<chain_length; i++) {
      printf("%d: %p %d flags %d\n", i, chain[i].message, chain[i].message_len, chain[i].flags);
    }
    virtio_process_control(dev, chain, chain_length, start_idx);
  }
}

static void cnfa_audio_cb(struct CNFADriver *dev, short *out, short *in, int framesp, int framesr) {
  if (framesp > 0) { // playback
    if (guest_playing) {
    } else {
      memset(out, 0, framesp * 2);
    }
  } else {
    printf("cnfa_audio_cb(%p, %p, %p, %d, %d)\n", dev, out, in, framesp, framesr);
  }
}

static const virtio_device_type virtio_snd_type = {
  .device_type = 25,
  .queue_count = 4,
  .config_load = virtio_snd_config_load,
  .config_store = virtio_snd_config_store,
  .process_command = virtio_snd_process_command,
};

struct virtio_device *virtio_snd_create(void *ram_image, uint32_t base) {
  audio_host = CNFAInit(NULL, "full-rv32ima", cnfa_audio_cb, 44100, 0, 1, 0, 1024, NULL, NULL, NULL);

  return virtio_create(ram_image, &virtio_snd_type, base, 0x200, get_next_irq());
}
