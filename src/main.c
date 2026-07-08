#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <alsa/asoundlib.h>

#include "defs.h"
#include "sampler.h"
#include "midi.h"
#include "keyboard.h"
#include "control_socket.h"

#define DEVICE "default"

#define KB      1
#define MIDI    2

#if KB_INPUT
    #define INPUT KB
#else
    #define INPUT MIDI
#endif

int main(int argc, char *argv[])
{

    /* Initializing the samples */
    /* Kick sample */
    sample_t sample_kick;
    uint8_t init_res = init_sample(&sample_kick, "Kick", "audio/kick.wav");
    if (init_res) return 1;

    /* Snare sample */
    sample_t sample_snare;
    init_res = init_sample(&sample_snare, "Snare", "audio/snare.wav");
    if (init_res) return 1;
    
    /* Hi-hat sample */
    sample_t sample_hat;
    init_res = init_sample(&sample_hat, "Hat", "audio/hat.wav");
    if (init_res) return 1;

    sample_t sample_perc;
    init_res = init_sample(&sample_perc, "Perc", "audio/perc.wav");
    if (init_res) return 1;

    sample_t sample_hat2;
    init_res = init_sample(&sample_hat2, "Hat2", "audio/hat2.wav");
    if (init_res) return 1;

    sample_t sample_guitar;
    init_res = init_sample(&sample_guitar, "Guitar", "audio/guitar.wav");
    if (init_res) return 1;

    sample_t sample_test;
    init_res = init_sample(&sample_test, "Test", "audio/test.wav");
    if (init_res) return 1;

    
    /* Adding the samples to the sampler */
    sampler_t sampler;
    init_res = init_sampler(&sampler);
    if (init_res) return 1;
    add_sample(&sampler, &sample_kick);
    add_sample(&sampler, &sample_snare);
    add_sample(&sampler, &sample_hat);
    add_sample(&sampler, &sample_hat2);
    add_sample(&sampler, &sample_perc);
    add_sample(&sampler, &sample_guitar);
    add_sample(&sampler, &sample_test);
    
    /* Initializing the ALSA PCM*/
    snd_pcm_t *pcm;
    snd_pcm_open(&pcm, DEVICE, SND_PCM_STREAM_PLAYBACK, 0);

    /* Initializing the ALSA PCM parameters */
    unsigned int rate = BITRATE;
    int channels = MONO;
    snd_pcm_uframes_t period_size = 256, buffer_size = FRAMES;    
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm, params);
    snd_pcm_hw_params_set_access(
        pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(
        pcm, params, SND_PCM_FORMAT_S32_LE);
    snd_pcm_hw_params_set_channels(
        pcm, params, channels);
    snd_pcm_hw_params_set_rate_near(
        pcm, params, &rate, NULL);
    snd_pcm_hw_params_set_period_size_near(
        pcm, params, &period_size, NULL);
    snd_pcm_hw_params_set_buffer_size_near(
        pcm, params, &buffer_size);
    snd_pcm_hw_params(pcm, params);

    /* Initializing the MIDI input */
    snd_rawmidi_t *midi_in;
    snd_rawmidi_open(&midi_in, NULL, "hw:4,0,0", SND_RAWMIDI_NONBLOCK);

    /* Initializing the sound buffer */
    uint32_t alsa_buffer_size = FRAMES * channels;
    int32_t buffer[alsa_buffer_size];

    int control_listen_fd = control_socket_open("/tmp/sampler.sock");
    int control_client_fd = -1;
    if (control_listen_fd < 0) return 1;
    

    /* Infinite loop */
    while (1) 
    {
        /* Getting the MIDI input from the controller */
        if (INPUT == MIDI)
            get_midi(midi_in, &sampler);
        else if (INPUT == KB)
        {
            init_kb();
            if (kbhit())
            {
                char c = getch();
                if (c == 'q')
                {
                    sampler.samples[0]->active = 1;
                    sampler.samples[0]->snd_buffer_pos = 0.0f;
                }
                else if (c == 's')
                {
                    sampler.samples[1]->active = 1;
                    sampler.samples[1]->snd_buffer_pos = 0.0f;
                }
                else if (c == 'd')
                {
                    sampler.samples[2]->active = 1;
                    sampler.samples[2]->snd_buffer_pos = 0.0f;
                }
                else if (c == 'f')
                {
                    sampler.samples[3]->active = 1;
                    sampler.samples[3]->snd_buffer_pos = 0.0f;
                }
                else if (c == 'g')
                {
                    sampler.samples[4]->active = 1;
                    sampler.samples[4]->snd_buffer_pos = 0.0f;
                }
                else if (c == 'h')
                {
                    sampler.samples[5]->active = 1;
                    sampler.samples[5]->snd_buffer_pos = 0.0f;
                }
            }
        }
        else fprintf(stderr, "ERROR: Bad input method.\n");

        if (control_listen_fd >= 0)
        {
            control_socket_accept(control_listen_fd, &control_client_fd);
            control_socket_poll(&control_client_fd, &sampler);
        }

        
        /* Processing the samples into the sound buffer */
        for (uint32_t i = 0; i < alsa_buffer_size; i++)
            buffer[i] = process_sampler(&sampler);

        /* Writing the sound buffer to the ALSA PCM */
        snd_pcm_sframes_t written =
            snd_pcm_writei(pcm, buffer, alsa_buffer_size);

        /* ALSA PCM write error handling */
        if (written < 0)
            written = snd_pcm_recover(pcm, written, 0);
        if (written < 0) 
        {
            fprintf(stderr, "Write failed: %s\n",
                    snd_strerror(written));
            break;
        }

        if (INPUT == KB)
            close_kb();
    }

    if (control_client_fd >= 0) close(control_client_fd);
    if (control_listen_fd >= 0) close(control_listen_fd);

    /* Close the ALSA PCM */
    snd_pcm_drain(pcm);
    snd_pcm_close(pcm);

    return 0;
}