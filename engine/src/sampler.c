/*##############################################################*/
/* Sample playback and initialization handling                  */
/* For the WAVE files loading and conversions, see wav_loader.c */
/*##############################################################*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <math.h>

#include "sampler.h"
#include "wav_loader.h"

/* MACROS */
#define INC_MOD(val, mod) val = (val + 1) % mod
#define CLIP_MAX(val, max) if (val > max) val = max
#define CLIP_MIN(val, min) if (val < min) val = min

/* Initialize a sample_t structure from a file name */
uint8_t init_sample(
    sample_t    *sample,        /* The sample to be initialized */
    const char  *sample_name,   /* The sample name */
    const char  *file_name)     /* The WAV file path */
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
    
    /* Loading the WAV file */
    wav_err_t err = wav_load_as_s32(file_name, sample->audio_data);

    /* WAV loading error */
    if (err != WAV_OK)
    {
        fprintf(stderr, "WAV LOADER ERROR: %s\nWAV FILE: %s\n", wav_strerror(err), file_name);
        return 1;
    }


    strcpy(sample->sample_name, sample_name);
    strcpy(sample->file_name, file_name);
    sample->snd_buffer_pos      = 0.0f;
    sample->pitch               = 1.0f;
    sample->active              = 0;
    sample->velocity            = 100;
    sample->buffer_start_pos    = 0;
    sample->buffer_end_pos      = sample->audio_data->num_frames;

    return 0;
}

uint8_t chop_sample(
    sample_t *sample,
    uint32_t start_pos,
    uint32_t end_pos)
{
    if (!sample) return 1;
    if (!sample->audio_data) return 1;
    if (start_pos > end_pos                         ||
        start_pos >= sample->audio_data->num_frames ||
        end_pos >= sample->audio_data->num_frames) return 1;
    sample->buffer_start_pos    = start_pos;
    sample->buffer_end_pos      = end_pos;
    return 0;
}

/* Process a sample frame */
int32_t process_sample(sample_t *sample)
{
    /* Safe guards */
    if (!sample || !sample->audio_data || !sample->audio_data->data)
        return 0;

    /* Getting the number of frames and the number of channels from the wav_audio_t */
    uint32_t num_frames     = sample->buffer_end_pos;
    uint16_t num_channels   = sample->audio_data->num_channels;

    /* If we finished the buffer */
    if (sample->snd_buffer_pos >= (float)num_frames)
    {
        /* Sample go inactive */
        sample->active = 0;
        sample->snd_buffer_pos = (float) sample->buffer_start_pos;
        return 0;
    }

    /* Getting the frame pointer from the frame index */
    uint32_t frame_index = (uint32_t)sample->snd_buffer_pos;
    const int32_t *frame_ptr = &sample->audio_data->data[frame_index * num_channels];

    /* Normalizing stereo to mono */
    int64_t val = 0;
    for (uint16_t c = 0; c < num_channels; c++)
        val += frame_ptr[c];
    val /= num_channels;

    /* Applying pitch change */
    sample->snd_buffer_pos += sample->pitch;

    /* Appying MIDI velocity and returning the sample frame */
    return (int32_t) ((float) val * (sample->velocity / 127.0f));
}

/* Initialize a sampler_t structure */
uint8_t init_sampler(sampler_t *sampler)
{
    /* Allocate memory space for the sample_t array */
    sampler->samples = malloc(sizeof(sample_t *) * SAMPLES);
    if (!sampler->samples) return 1;
    for (uint8_t i = 0; i < SAMPLES; i++)
    {
        sampler->samples[i] = malloc(sizeof(sample_t));
        if (!sampler->samples[i]) return 1;
    }

    /* Assigning base values */
    sampler->sample_count = 0;
    sampler->global_pitch = 24;
    
    return 0;
}

/* Processing a sampler_t frame by processing all the sample_t frames and mixing them together */
int32_t process_sampler(sampler_t *sampler)
{
    /* Safe guards */
    if (!sampler)                   return 1;
    if (!sampler->samples)          return 1;
    if (sampler->sample_count == 0) return 1;

    /* 64 bit sample for mixing */
    int64_t snd_sample = 0;

    /* Looping on the sample_t */
    uint8_t active_samples = 0;
    for (uint8_t s = 0; s < sampler->sample_count; s++)
        if (sampler->samples[s]->active)
        {
            /* Processing the sample_t frame */
            snd_sample += process_sample(sampler->samples[s]);
            active_samples++;
        }
    /* Calculating the gain from the number of active samples */
    float gain = (active_samples > 0) ? 1.0f / sqrt((float) active_samples) : 0.0f;
    float processed_sample = (float) snd_sample * gain;
    snd_sample = (int32_t) processed_sample;

    /* Clipping */
    CLIP_MAX(snd_sample, INT32_MAX);
    CLIP_MIN(snd_sample, INT32_MIN);

    return (int32_t) snd_sample;
}

uint8_t get_sample_idx(sampler_t *sampler, const char *sample_name)
{
    if (!sampler) return -1;
    if (!sampler->samples) return -1;
    for (int i = 0; i < sampler->sample_count; i++)
    {
        sample_t *s = sampler->samples[i];
        
        if (!s) return -1;
        if (strcmp(s->sample_name, sample_name) == 0)
        {
            //fprintf(stderr, "%s : %s : %s", s->file_name, s->sample_name, sample_name);
            return i;
        }
            
    }
    return -1;
}