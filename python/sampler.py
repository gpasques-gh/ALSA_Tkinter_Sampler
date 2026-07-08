import socket
import tkinter as tk


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
    

client = SamplerClient()

root = tk.Tk()

textbox_filename = tk.Entry(root, width=25)
textbox_samplename = tk.Entry(root, width=25)
button_add_sample = tk.Button(root, text="Add sample", width=25, command=lambda:client.add_sample(textbox_samplename.get(), textbox_filename.get()))

textbox_samplename.pack()
textbox_filename.pack()
button_add_sample.pack()

root.mainloop()
