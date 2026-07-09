#include "wav_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WAVE_FORMAT_PCM         0x0001
#define WAVE_FORMAT_IEEE_FLOAT  0x0003
#define WAVE_FORMAT_ALAW        0x0006
#define WAVE_FORMAT_MULAW       0x0007
#define WAVE_FORMAT_EXTENSIBLE  0xFFFE

typedef struct
{
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint16_t valid_bits;
} wav_fmt_t;


/* Little endian readers */
static uint16_t rd_u16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t rd_u32(const uint8_t *p)
{
    return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24));
}

static int32_t rd_s32(const uint8_t *p) { return (int32_t)rd_u32(p); }

const char *wav_strerror(wav_err_t err)
{
    switch(err)
    {
    case WAV_OK:                        return "ok";
    case WAV_ERR_OPEN:                  return "could not open file";
    case WAV_ERR_READ:                  return "read error";
    case WAV_ERR_NOT_RIFF:              return "missing RIFF header";
    case WAV_ERR_NOT_WAVE:              return "not a WAVE file";
    case WAV_ERR_NO_FMT:                return "missing fmt chunk";
    case WAV_ERR_NO_DATA:               return "missing data chunk";
    case WAV_ERR_UNSUPPORTED_FORMAT:    return "unsupported sample format";
    case WAV_ERR_BAD_FMT_SIZE:          return "malformed fmt chunk";
    case WAV_ERR_OOM:                   return "out of memory";
    }
    return "unkown error";
}

/* Sample conversion */

static int32_t conv_u8(const uint8_t *p)
{
    int32_t v = (int32_t)p[0] - 128;
    return v << 24;
}

static int32_t conv_s16(const uint8_t *p)
{
    int16_t v = (int16_t)rd_u16(p);
    return ((int32_t)v) << 16;
}

static int32_t conv_s24(const uint8_t *p)
{
    int32_t v = (int32_t)(p[0] | (p[1] << 8) | (p[2] << 16));
    if (v & 0x00800000) v |= (int32_t)0xFF000000;
    return v << 8;
}

static int32_t conv_s32(const uint8_t *p)
{
    return rd_s32(p);
}

static int32_t conv_f32(const uint8_t *p)
{
    union { uint32_t u; float f; } u;
    u.u = rd_u32(p);
    double v = (double)u.f;
    if (v > 1.0)    v = 1.0;
    if (v < -1.0)   v = -1.0;
    return (int32_t)lround(v * 2147483647.0); 
}

static int32_t conv_f64(const uint8_t *p)
{
    uint64_t bits = 0;
    for (int i = 0; i < 8; i++) bits |= ((uint64_t)p[i]) << (8 * i);
    union { uint64_t u; double d; } u;
    u.u = bits;
    double v = u.d;
    if (v > 1.0)    v = 1.0;
    if (v < -1.0)   v = -1.0;
    return (int32_t)lround(v * 2147483647.0);
}

typedef int32_t (*sample_conv_fn)(const uint8_t *);

static int pick_converter(
    const wav_fmt_t *fmt, 
    sample_conv_fn *conv,
    int *bytes)
{
    if (fmt->audio_format == WAVE_FORMAT_PCM)
    {
        switch (fmt->bits_per_sample)
        {
        case 8:     *conv = conv_u8;    *bytes = 1; return 0;
        case 16:    *conv = conv_s16;   *bytes = 2; return 0;
        case 24:    *conv = conv_s24;   *bytes = 3; return 0;
        case 32:    *conv = conv_s32;   *bytes = 4; return 0;
        default:    return -1;
        }
    }
    else if (fmt->audio_format == WAVE_FORMAT_IEEE_FLOAT)
    {
        switch (fmt->bits_per_sample)
        {
        case 32:    *conv = conv_f32;   *bytes = 4; return 0;
        case 64:    *conv = conv_f64;   *bytes = 8; return 0;
        default:    return -1;
        }
    }

    return -1;
}

wav_err_t wav_load_as_s32(
    const char *path,
    wav_audio_t *out)
{

    memset(out, 0, sizeof(*out));

    /* Open WAV file */
    FILE *f = fopen(path, "rb");
    if (!f) return WAV_ERR_OPEN;

    /* Check if file is valid and get its size */
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return WAV_ERR_READ; }
    long fsize = ftell(f);
    if (fsize < 12) { fclose(f); return WAV_ERR_READ; }
    rewind(f);

    /* Read the file int an unsigned char buffer */
    uint8_t *buffer = malloc((size_t)fsize);
    if (!buffer) { fclose(f); return WAV_ERR_OOM; }
    if (fread(buffer, 1, (size_t)fsize, f) != (size_t)fsize)
    {
        free(buffer);
        fclose(f);
        return WAV_ERR_READ;
    }
    fclose(f);

    if (memcmp(buffer, "RIFF", 4) != 0) { free(buffer); return WAV_ERR_NOT_RIFF; }
    if (memcmp(buffer + 8, "WAVE", 4) != 0) { free(buffer); return WAV_ERR_NOT_WAVE; }

    /* Initialize the FMT struct */
    wav_fmt_t fmt = {0};
    int have_fmt = 0;
    const uint8_t *data_ptr = NULL;
    uint32_t data_size = 0;

    size_t pos = 12;
    while (pos + 8 <= (size_t)fsize)
    {
        /* Getting the chunk from the buffer */
        const uint8_t *chunk_id = buffer + pos;
        uint32_t chunk_size = rd_u32(buffer + pos + 4);
        const uint8_t *chunk_data = buffer + pos + 8;

        /* Guard against a corrupt size running past EOF */
        if (pos + 8 + (size_t)chunk_size > (size_t)fsize) break;

        if (memcmp(chunk_id, "fmt ", 4) == 0)
        {
            if (chunk_size < 16) { free(buffer); return WAV_ERR_BAD_FMT_SIZE; }

            /* Getting the FMT parameters */
            fmt.audio_format    = rd_u16(chunk_data + 0);
            fmt.num_channels    = rd_u16(chunk_data + 2);
            fmt.sample_rate     = rd_u32(chunk_data + 4);
            fmt.byte_rate       = rd_u32(chunk_data + 8);
            fmt.block_align     = rd_u16(chunk_data + 12);
            fmt.bits_per_sample = rd_u16(chunk_data + 14);
            fmt.valid_bits      = fmt.bits_per_sample;

            if (fmt.audio_format == WAVE_FORMAT_EXTENSIBLE)
            {
                if (chunk_size < 40) { free(buffer); return WAV_ERR_BAD_FMT_SIZE; }
                uint16_t valid_bits = rd_u16(chunk_data + 18);
                if (valid_bits) fmt.valid_bits = valid_bits;
                fmt.audio_format = rd_u16(chunk_data + 24);
            }

            have_fmt = 1;
        }
        else if (memcmp(chunk_id, "data", 4) == 0)
        {
            data_ptr = chunk_data;
            data_size = chunk_size;
        }

        pos += 8 + chunk_size;
        if (chunk_size & 1) pos += 1;
    }

    /* Handling errorrs */
    if (!have_fmt) { free(buffer); return WAV_ERR_NO_FMT; }
    if (!data_ptr) { free(buffer); return WAV_ERR_NO_DATA; }
    if (fmt.num_channels == 0 || fmt.block_align == 0)
    {
        free(buffer);
        return WAV_ERR_BAD_FMT_SIZE;
    }

    /* Get the right converter for the file */
    sample_conv_fn conv;
    int src_bytes;
    if (pick_converter(&fmt, &conv, &src_bytes) != 0) { free(buffer); return WAV_ERR_UNSUPPORTED_FORMAT; }

    /* Get the number of frames of the sample */
    uint32_t num_frames = data_size / fmt.block_align;
    size_t total_samples = (size_t)num_frames * fmt.num_channels;

    /* Initialize the data pointer */
    int32_t *out_data = malloc(total_samples * sizeof(int32_t));
    if (!out_data) { free(buffer); return WAV_ERR_OOM; }

    /* Convert the data */
    const uint8_t *src = data_ptr;
    for (size_t i = 0; i < total_samples; i++)
    {
        out_data[i] = conv(src);
        src += src_bytes;
    }

    free(buffer);

    /* Assigning the values to the wav_audio_t pointer */
    out->data           = out_data;
    out->num_frames     = num_frames;
    out->num_channels   = fmt.num_channels;
    out->sample_rate    = fmt.sample_rate;
    
    return WAV_OK;
}