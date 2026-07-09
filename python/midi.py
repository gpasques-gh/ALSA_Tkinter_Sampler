import rtmidi
from sampler_client import SamplerClient

# MIDI handler that trigger the samples using its SamplerClient object
class MidiHandler:
    def __init__(self, midi_device: int, sampler_client: SamplerClient):
        self.midi_input = rtmidi.MidiIn()
        self.midi_input.open_port(midi_device)
        self.midi_input.set_callback(self.callback)
        self.sampler_client = sampler_client

    def callback(self, msg: list, timestamp: float):
        if msg[0][0] == 0x90:
            note, velocity = msg[0][1], msg[0][2]
            if (note - 0x20 >= 0): note -= 0x20
            self.sampler_client.trigger(note, velocity=velocity)