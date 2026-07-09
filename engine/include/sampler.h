#ifndef SAMPLER_H
#define SAMPLER_H
#include <stdint.h>
#include "defs.h"
#include "wav_loader.h"

/* STRUCTURES 
- sample_t  -> sample structure
- sampler_t -> sampler structure */

/* SAMPLE STRUCTURE
- Each sample from the sampler is read by 512 16 bytes chunks into the ALSA sound card
- We are keeping the current index of each sample buffer
*/
typedef struct
{
    const char  *sample_name;      /* Name of the sample */
    const char  *file_name;        /* Filename of the sample file */
    wav_audio_t *audio_data;        /* Audio data */
    float       snd_buffer_pos;     /* Current index of the sound buffer */
    float       pitch;              /* MIDI pitch (from 0 to 127), base sample pitch being 24 (C3) */
    uint8_t     active;             /* If the sound is to be played or not */
    uint8_t     velocity;           /* Velocity of the MIDI note */
} sample_t;

/* SAMPLER STRUCTURE */
typedef struct 
{
    sample_t    **samples;          /* Samples array of the sampler, length is defined in defs.h */
    uint8_t     global_pitch;       /* Global pitch of the sampler */
    uint8_t     sample_count;       /* Number of loaded samples */
} sampler_t;

/* FUNCTIONS */
/* All functions returns 0 on success and 1 on failure */

/* SAMPLE FUNCTIONS */

/* Initialize a sample structure from a file name */
uint8_t init_sample(
    sample_t         *sample, 
    const char   *sample_name,
    const char   *file_name);

/* Change a sample pitch */
uint8_t change_sample_pitch(
    sample_t    *sample,
    uint8_t     picth); 

/* Read a sample into the ALSA sound buffer */
int32_t process_sample(sample_t *sample);

/* SAMPLER FUNCTIONS */

/* Initialize an empty sampler */
uint8_t init_sampler(sampler_t *sampler);

/* Change to global pitch of the sampler */
uint8_t change_global_pitch(
    sampler_t   *sampler,
    uint8_t     global_pitch);

/* Read the sampler into the ALSA sound buffer */
int32_t process_sampler(sampler_t   *sampler);

#endif 