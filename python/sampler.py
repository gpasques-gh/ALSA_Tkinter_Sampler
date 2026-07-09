import socket
import tkinter as tk
import rtmidi

# Socket client to communicate with the C sampler
class SamplerClient:
    def __init__(self, path="/tmp/sampler.sock"):
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(path)

    def trigger(self, idx, velocity=127):
        self.sock.sendall(f"TRIGGER {idx} {velocity}\n".encode())

    def set_pitch(self, idx, pitch):
        self.sock.sendall(f"SET_PITCH {idx} {pitch}\n".encode())

    def add_sample(self, sample_name, file_name):
        self.sock.sendall(f"ADD_SAMPLE {sample_name} {file_name}\n".encode())

    def status(self):
        self.sock.sendall(b"STATUS\n")
        return self.sock.recv(4096).decode()
    

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
        
client = SamplerClient()
midi_handler = MidiHandler(4, client)

client.add_sample("kick", "audio/kick.wav")
client.add_sample("snare", "audio/snare.wav")
client.add_sample("hat", "audio/hat.wav")
client.add_sample("guitar", "audio/guitar.wav")
client.add_sample("bass", "audio/bass.wav")

root = tk.Tk()

textbox_filename = tk.Entry(root, width=25)
textbox_samplename = tk.Entry(root, width=25)
button_add_sample = tk.Button(root, text="Add sample", width=25, command=lambda:client.add_sample(textbox_samplename.get(), textbox_filename.get()))
button_status = tk.Button(root, text="Status", width=25, command=lambda:client.status())

textbox_samplename.pack()
textbox_filename.pack()
button_add_sample.pack()
button_status.pack()

root.mainloop()
