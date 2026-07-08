#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <math.h>
#include "sampler.h"
#include "wav_loader.h"

#define INC_MOD(val, mod) val = (val + 1) % mod
#define CLIP_MAX(val, max) if (val > max) val = max
#define CLIP_MIN(val, min) if (val < min) val = min

static long test = 0;

uint8_t init_sample(
    sample_t    *sample,
    const char  *sample_name,
    const char  *file_name)
{
    /* Checking if the pointers are valid */
    if (sample      == NULL || 
        sample_name == NULL || 
        file_name   == NULL) {
            fprintf(stderr, "SAMPLE INITIALIZATION ERROR : Invalid pointers given.\n");
            return 1;
    }

    /* Getting the sample informations */
    sample->audio_data = malloc(sizeof(wav_audio_t));
    sample->file_name       = file_name;
    sample->sample_name     = sample_name;
    sample->snd_buffer_pos  = 0.0f;
    sample->pitch           = 1.0f;
    sample->active          = 0;
    sample->velocity        = 100;

    /* Loading the WAV file */
    wav_err_t err = wav_load_as_s32(file_name, sample->audio_data);

    /* WAV loading error */
    if (err != WAV_OK)
    {
        fprintf(stderr, "WAV LOADER ERROR: %s\nWAV FILE: %s\n", wav_strerror(err), file_name);
        return 1;
    }

    return 0;
}

int32_t process_sample(sample_t *sample)
{
    if (!sample || !sample->audio_data || !sample->audio_data->data)
        return 0;

    uint32_t num_frames     = sample->audio_data->num_frames;
    uint16_t num_channels   = sample->audio_data->num_channels;

    if (sample->snd_buffer_pos >= (float)num_frames)
    {
        sample->active = 0;
        sample->snd_buffer_pos = 0.0f;
        return 0;
    }

    uint32_t frame_index = (uint32_t)sample->snd_buffer_pos;
    const int32_t *frame_ptr = &sample->audio_data->data[frame_index * num_channels];
    int64_t val = 0;
    for (uint16_t c = 0; c < num_channels; c++)
        val += frame_ptr[c];
    val /= num_channels;
    sample->snd_buffer_pos += sample->pitch;
    return (int32_t) ((float) val * (sample->velocity / 127.0f));
}

uint8_t init_sampler(sampler_t *sampler)
{
    sampler->samples = malloc(sizeof(sample_t *) * SAMPLES);
    if (!sampler->samples) return 1;
    for (uint8_t i = 0; i < SAMPLES; i++)
    {
        sampler->samples[i] = malloc(sizeof(sample_t));
        if (!sampler->samples[i]) return 1;
    }
    sampler->sample_count = 0;
    sampler->global_pitch = 24;
    return 0;
}

uint8_t add_sample(
    sampler_t   *sampler,
    sample_t    *sample)
{
    /* Checking if the pointers are valid */
    if (!sampler || !sample) return 1;

    /* If the sampler is not initialized, try to initialize it */
    if (!sampler->samples)
    {
        uint8_t init_res = init_sampler(sampler);
        if (init_res) return 1;
    }

    /* Add the sample to the sampler */
    if (sampler->sample_count >= SAMPLES) return 1;
    sampler->sample_count++;
    sampler->samples[sampler->sample_count - 1] = sample;
    return 0;
}

int32_t process_sampler(sampler_t *sampler)
{
    if (!sampler)                   return 1;
    if (!sampler->samples)          return 1;
    if (sampler->sample_count == 0) return 1;

    int64_t snd_sample = 0;

    uint8_t active_samples = 0;
    for (uint8_t s = 0; s < sampler->sample_count; s++)
        if (sampler->samples[s]->active)
        {
            snd_sample += process_sample(sampler->samples[s]);
            active_samples++;
        }
    float gain = (active_samples > 0) ? 1.0f / sqrt((float) active_samples) : 0.0f;
    float processed_sample = (float) snd_sample * gain;
    snd_sample = (int32_t) processed_sample;
    CLIP_MAX(snd_sample, INT32_MAX);
    CLIP_MIN(snd_sample, INT32_MIN);
    return (int32_t) snd_sample;
}