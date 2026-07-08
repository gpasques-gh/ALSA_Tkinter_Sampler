#ifndef MIDI_H
#define MIDI_H

#include <alsa/asoundlib.h>
#include "sampler.h"

uint8_t get_midi(snd_rawmidi_t *midi_in, sampler_t *sampler);

#endif