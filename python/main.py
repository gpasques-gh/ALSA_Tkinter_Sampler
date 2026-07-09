import tkinter as tk
from tkinter import messagebox
import os
import signal
from time import sleep

from midi import MidiHandler
from sampler_client import SamplerClient

# Closing function
def on_closing():
    if messagebox.askokcancel("Quit", "Do you want to quit?"):
        os.system("killall socat 1>/dev/null 2>&1")
        os.system("killall sampler 1>/dev/null 2>&1")
        root.destroy()

# SIGINT handler
def handle_sigint(signum, frame):
    on_closing()
signal.signal(signal.SIGINT, handle_sigint)

# Keep alive function to intercept SIGINT properly
def keep_alive():
    root.after(100, keep_alive)

# Launching the C program
os.system("./bin/sampler 1>/dev/null 2>&1 &")

# Making the relay from the AF_INET socket to the UNIX socket 
# (useful to run the program on Windows with WSL)
os.system("socat TCP-LISTEN:9999,fork,reuseaddr UNIX-CONNECT:/tmp/sampler.sock &")
sleep(0.5)
        
# Initializing the sampler client and the MIDI handler
client = SamplerClient()
midi_handler = MidiHandler(4, client)

# Adding some test samples (TO REMOVE)
client.add_sample("kick", "audio/kick.wav")
client.add_sample("snare", "audio/snare.wav")
client.add_sample("hat", "audio/hat.wav")
client.add_sample("guitar", "audio/guitar.wav")
client.add_sample("bass", "audio/bass.wav")

# Creating the Tkinter window
root = tk.Tk()

# Exiting functions, keep alive to intercept SIGINT properly
root.protocol("WM_DELETE_WINDOW", on_closing)
root.after(100, keep_alive)

textbox_filename = tk.Entry(root, width=25)
textbox_samplename = tk.Entry(root, width=25)
button_add_sample = tk.Button(root, text="Add sample", width=25, command=lambda:client.add_sample(textbox_samplename.get(), textbox_filename.get()))
button_status = tk.Button(root, text="Status", width=25, command=lambda:client.status())

textbox_samplename.pack()
textbox_filename.pack()
button_add_sample.pack()
button_status.pack()

root.mainloop()
