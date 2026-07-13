from sampler_client import SamplerClient

class Sample:
    def __init__(self, sample_name:str, file_name:str):
        self.sample_name:str = sample_name
        self.file_name:str = file_name
        self.pitch:float = 1.0
        self.velocity:int = 127

    def play(self, velocity:int, client:SamplerClient):
        client.trigger_name(self.sample_name, velocity=velocity)

    def set_pitch(self, pitch:float, client:SamplerClient):
        self.pitch += pitch
        client.set_pitch_name(self.sample_name, pitch)


class Sampler:
    def __init__(self):
        self.samples:list = []
        
    def play_sample(self, sample_name:str, velocity:int, client:SamplerClient):
        if sample_name not in self.samples:
            return 1
        for sample in enumerate(self.samples):
            if (sample.sample_name == sample_name):
                sample.play(velocity, client)
        return 0
        
    def add_sample(self, sample:Sample, client:SamplerClient):
        if (len(self.samples) > 11):
            return 1
        self.samples.append(sample)
        client.add_sample(sample.sample_name, sample.file_name)