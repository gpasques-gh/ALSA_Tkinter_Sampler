import socket

# Socket client to communicate with the C sampler
class SamplerClient:
    def __init__(self, host="127.0.0.1", port=9999):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((host, port))

    def trigger(self, idx, velocity=127):
        self.sock.sendall(f"TRIGGER {idx} {velocity}\n".encode())

    def set_pitch(self, idx, pitch):
        self.sock.sendall(f"SET_PITCH {idx} {pitch}\n".encode())

    def add_sample(self, sample_name, file_name):
        self.sock.sendall(f"ADD_SAMPLE {sample_name} {file_name}\n".encode())

    def status(self):
        self.sock.sendall(b"STATUS\n")
        return self.sock.recv(4096).decode()