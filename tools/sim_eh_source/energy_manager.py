"""
Generates energy at a certain position, and point in time, based on
* energy source
* set of shaders
"""
from threading import Lock

class EnergyManager():
    def __init__(self, e_source, shaders):
        self.e_source = e_source
        self.shaders = shaders
        self.crt_time = 0
        self.eman_lock = Lock()

    def get_energy(self, position, start_time, end_time):
        """
        Returns the energy generated at position from start time to end_time
        """
        # Update current time
        if start_time > self.crt_time:
            self.crt_time = start_time
        # Retrieve raw energy from the source
        input_energy = self.e_source.between(start_time, end_time)
        # How much attenuation is applied?
        # Synchronise access towards shader historic
        self.eman_lock.acquire()
        attenuation = min([s.get_attenuation(position, start_time, end_time)\
                                for s in self.shaders])
        self.eman_lock.release()
        #print position, start_time, end_time, input_energy, attenuation
        return input_energy*attenuation

