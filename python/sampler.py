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

    def status(self):
        self.sock.sendall(b"STATUS\n")
        return self.sock.recv(4096).decode()
    

client = SamplerClient()

root = tk.Tk()

button = tk.Button(root, text="Kick", width=25, command=lambda:client.trigger(5, 127))
button_pitch = tk.Button(root, text="Kick_pitch", width=25, command=lambda:client.set_pitch(5, -0.1))
button_pitch_up = tk.Button(root, text="Pitch up", width=25, command=lambda:client.set_pitch(5, 0.1))
button.pack()
button_pitch.pack()
button_pitch_up.pack()
root.mainloop()
