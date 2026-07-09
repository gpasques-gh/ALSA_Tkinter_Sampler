#ifndef WAV_LOADER_H
#define WAV_LOADER_H

#include <stdint.h>
#include <stddef.h>

typedef struct
{
    int32_t *data;          /* Interleaved samples */
    uint32_t num_frames;    /* Frames = samples per channel */
    uint16_t num_channels;
    uint32_t sample_rate;
} wav_audio_t;

typedef enum
{
    WAV_OK = 0,
    WAV_ERR_OPEN,
    WAV_ERR_READ,
    WAV_ERR_NOT_RIFF,
    WAV_ERR_NOT_WAVE,
    WAV_ERR_NO_FMT,
    WAV_ERR_NO_DATA,
    WAV_ERR_UNSUPPORTED_FORMAT,
    WAV_ERR_BAD_FMT_SIZE,
    WAV_ERR_OOM,
} wav_err_t;

/* Loads path, parses RIFF/fmt/data chunks, converts every sample to S32 */
/* Returns WAV_OK and fills *out  on sucees (out->data must be free()'d by caller) */
/* On failure return an error code and *out is left zeroed */
wav_err_t wav_load_as_s32(const char *path, wav_audio_t *out);

/* Human readable string for wav_err_t */
const char *wav_strerror(wav_err_t err);

#endif /* WAV_LOADER_H */