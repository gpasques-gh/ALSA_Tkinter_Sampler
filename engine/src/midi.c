/*######################################################################*/
/* WARNING : This is a legacy file using ALSA RAWMIDI interface         */
/* MIDI communication is now done through the Python socket interface   */
/*######################################################################*/

#include <alsa/asoundlib.h>
#include "defs.h"
#include "sampler.h"
#include "midi.h"

#if AKAI_MPK_PADS
    #define BASE 0x20
#else
    #define BASE 0
#endif

uint8_t get_midi(snd_rawmidi_t *midi_in, sampler_t *sampler)
{
    if (!midi_in || !sampler)   return 1;
    if (!sampler->samples)      return 1;
    uint8_t midi_buffer[1024];
    ssize_t ret = snd_rawmidi_read(midi_in, midi_buffer, sizeof(midi_buffer));

    if (ret < 0) return 1;

    for (int i = 0; i + 2 < ret; i += 3)
    {
        uint8_t status  = midi_buffer[i];
        uint8_t data_1  = midi_buffer[i + 1];
        uint8_t data_2  = midi_buffer[i + 2];

        if ((status & PRESSED) == NOTE_ON && data_2 > 0)
        {
            
            if (data_1 - BASE < sampler->sample_count)
            {
                sampler->samples[data_1 - BASE]->active = 1;
                sampler->samples[data_1 - BASE]->velocity = data_2;
                sampler->samples[data_1 - BASE]->snd_buffer_pos = 0.0f;
            }
        }
        else if ((status & PRESSED) == NOTE_OFF || data_2 <= 0)
        {
            
        }
    }
    return 0;
}