"""
Applies a shading pattern to an area
"""
from position import Rectangle

class Shading():
    def __init__(self, area, start, end):
        self.area = area
        self.start_time = start
        self.end_time = end
        #print "Shader at %s from %d to %d" %(str(area), start, end)

    def is_covering(self, position, time, end_time):
        """Determines if this shading covers the position"""
        contains = self.area.contains(position)
        #if contains:
        #    print "%d-%d Shader at %s from %d to %d covers %s" %(time, end_time,
        #            str(self.area), self.start_time, self.end_time, str(position))
        return contains

    def __repr__(self):
        return "Shader %s: %d->%d" % (str(self.area), self.start_time, self.end_time)

class CyclicShadingPattern():
    def __init__(self, area, start_time, end_time, cycle, attenuation):
        """
        A shader that happens periodically, each time in the same location.
        The period is a cycle, and within a cycle, the shade will always
        start at the same time and end at the same time.
        So, start_time and end_time must be < cycle.
        """
        self.area = area
        self.start_time = start_time
        self.end_time = end_time
        self.cycle = cycle
        self.attenuation = attenuation

    def attenuation_for(self, position):
        if self.area.contains(position):
            return self.attenuation
        else:
            return 1

    def get_attenuation(self, position, start_time, end_time):
        """
        Determines how much this shader attenuates the given position
        between the given start_time and end_time.
        """
        # Is this shader active during the start_time -> end_time interval?
        start_in_cycle = start_time % self.cycle
        end_in_cycle = end_time % self.cycle

        # TODO - For simplicity, assume large cycles, much larger than this duration
        # TODO - So, assume duration is less than a cycle
        
        if end_in_cycle < start_in_cycle: return 1 # no attenuation at midnight

        attenuation = 0
        if start_in_cycle < self.start_time:
            attenuation += 1 * (min(end_in_cycle, self.start_time) - start_in_cycle)
            start_in_cycle = min(end_in_cycle, self.start_time)
            attenuation += self.attenuation_for(position)\
                            * (end_in_cycle - start_in_cycle)
        elif start_in_cycle < self.end_time:
            attenuation += self.attenuation_for(position)\
                            * (min(end_in_cycle, self.end_time) - start_in_cycle)
            start_in_cycle = min(end_in_cycle, self.end_time)
            attenuation += 1 * (end_in_cycle - start_in_cycle)
        else:
            return 1

        return attenuation/(end_time - start_time)




class ShadingPattern():
    def __init__(self, area, size, period, duration, offset, attenuation):
        """
        area        -- the whole environment
        size        -- size of this shade, which will be square
        period      -- period between two consecutive shades
        duration    -- length of this shade, must be < than period
        offset      -- when the first shade will happen
        attenuation -- multiplier for the energy generated
        """
        # List of past shades
        self.history = []
        self.crt_time = 0
        self.area = area            # The environment area where we shade
        self.size = size            # How big is the shade (square)
        self.period = period        # How often does this pattern occur
        self.duration = duration
        self.attenuation = attenuation
        self.offset = offset
        # Must generate the first shade, for simplicity
        shade_pos = self.area.new_interior_point()
        shade_start = offset
        self.history.append(Shading(
                            Rectangle(shade_pos, self.size, self.size),
                            shade_start,
                            shade_start + self.duration))

    @classmethod
    def generate_shader_list(cls, area, size, period, duration, attenuation, offset, lifetime):
        """
        Generate a list of shaders covering the entire lifetime
        with the given parameters.

        Return  -- list of shaders
        """
        shaders = []
        while offset < lifetime:
            shade_pos = area.new_interior_point()
            shade_start = offset
            shaders.append(Shading(
                                Rectangle(shade_pos, size, size),
                                shade_start,
                                shade_start + duration))
            offset = shade_start + period
        return shaders
            
    def set_history(self, shaders):
        """
        Presets the history for this shader.
        """
        self.history = shaders

    def get_attenuation_value(self, shader, position, time, end_time):
        """
        If the position is covered by the shader, returns the attenuation value.
        Otherwise, returns 1 (not attenuated).
        """
        if shader.is_covering(position, time, end_time): return self.attenuation
        else: return 1

    def get_attenuation(self, position, start_time, end_time):
        """
        Determines if the specified position is shaded between start_time and
        duration, and returns the attenuation value, which is the average over
        that period
        """
        duration = end_time - start_time
        # First update current time
        if start_time > self.crt_time:
            self.crt_time = start_time
        # Is this time in the future (beyond the current shading?)
        if start_time > self.history[-1].end_time:
            # How many shadings are needed to cover duration?
            num_shadings = (end_time - self.history[-1].end_time)/self.period + 1
            for i in xrange(num_shadings):
                # Yup - new shade required
                shade_pos = self.area.new_interior_point()
                # TODO - should we randomise the period?
                shade_start = self.history[-1].start_time + (i+1)*self.period
                self.history.append(Shading(
                                    Rectangle(shade_pos, self.size, self.size),
                                    shade_start,
                                    shade_start + self.duration))
        # Go through all the shadings to see where we fit
        avg_attenuation = 0
        # When we find the end time, fix it so we only check for start thereon
        end_time_fixed = False
        for s in reversed(self.history):
            # Do we overlap in time with this shading?
            if not end_time_fixed:
                if end_time >= s.end_time:
                    avg_attenuation += 1*(end_time - max(s.end_time, start_time))
                    end_time_fixed = True
                    end_time = max(s.end_time, start_time)
                elif end_time >= s.start_time:
                    attenuation = self.get_attenuation_value(s, position, start_time, end_time)
                    avg_attenuation += attenuation * (end_time - max(s.start_time, start_time))
                    end_time_fixed = True
                    end_time = max(s.start_time, start_time)
                else:
                    # We don't overlap here
                    continue
            if end_time == start_time:
                # No more overlap
                break
            if start_time >= s.end_time:
                avg_attenuation += 1*(end_time - start_time)
                break
            else:
                # First add the non-attenuated part
                avg_attenuation += 1*(end_time - s.end_time)
                # Then check how much we overlap with the shaded part
                if start_time >= s.start_time:
                    attenuation = self.get_attenuation_value(s, position, start_time, end_time)
                    avg_attenuation += attenuation * (s.end_time - start_time)
                    start_time = end_time
                    break
                else:
                    # Start is before start of this shading, add all of it
                    attenuation = self.get_attenuation_value(s, position, start_time, end_time)
                    avg_attenuation += attenuation * (s.end_time - s.start_time)
                    end_time = s.start_time
        # In case we didn't fit anywhere (before any shaders)
        if start_time < end_time:
            avg_attenuation += 1 * (end_time - start_time)
        return avg_attenuation/float(duration)


def generate_cyclic_shaders(csc_file, num_shaders, cycle, pickle_file):
    """
    Generate a list of shading patterns with predefined historic.
    """
    from csc_parser import parse_csc
    from random import random, randint
    import pickle
    #############
    # What is the deployment area?
    net = parse_csc(csc_file)
    netcoords = zip(*net)[1]
    depl_orig_x = min(zip(*netcoords)[0])
    depl_orig_y = min(zip(*netcoords)[1])
    width = max(zip(*netcoords)[0]) - depl_orig_x
    height = max(zip(*netcoords)[1]) - depl_orig_y
    depl_area = Rectangle((depl_orig_x, depl_orig_y),
                          width, height)
    size = min(width, height)

    shaders = []
    for s in range(num_shaders):
        shade_origin = depl_area.new_interior_point()
        shade_size = (0.1+random()*0.3)*size      # Shade size is \in [10%,40%] of area min size
        shade_start = (6 + int(random()*10))*3600  # Shade starts bw 6AM-4PM
        shade_end = shade_start + (3 + int(random()*3))*3600    # Shade lasts 3-6h
        shade_attenuation = random()
        shaders.append(CyclicShadingPattern(Rectangle(shade_origin, shade_size, shade_size),
                                            shade_start, shade_end, cycle, shade_attenuation))
        print "New shader:", shade_origin, shade_size, shade_start, shade_end, shade_attenuation
    pickle.dump(shaders, open(pickle_file, 'w'))


def generate_shaders(csc_file, num_shaders, lifetime, pickle_file):
    """
    Generate a list of shading patterns with predefined historic.
    """
    from csc_parser import parse_csc
    from random import random, randint
    import pickle
    #############
    # What is the deployment area?
    net = parse_csc(csc_file)
    netcoords = zip(*net)[1]
    depl_orig_x = min(zip(*netcoords)[0])
    depl_orig_y = min(zip(*netcoords)[1])
    width = max(zip(*netcoords)[0]) - depl_orig_x
    height = max(zip(*netcoords)[1]) - depl_orig_y
    depl_area = Rectangle((depl_orig_x, depl_orig_y),
                          width, height)

    # Generate the shading patterns
    size = min(width, height)
    shaders = []
    for s in xrange(2):
        shade_size = (0.1+random()*0.3)*size      # Shade size is \in [5%,10%] of area min size
        shade_duration = int(random()*5*3600)     # Shade length is less than 5 hours
        shade_period = [6,24][randint(0,1)]*3600  # Period is either 6 (twice daily) or 24 hours (daily)
        shade_attenuation = random()
        shade_offset = (6 + randint(0,4))*3600    # EHTrace starts at midnight, so offset by at least 6h
        new_shade = ShadingPattern(
                                depl_area,
                                shade_size,
                                shade_period,
                                shade_duration,
                                shade_offset,
                                random())
        new_shade.set_history(ShadingPattern.generate_shader_list(
                                                    depl_area,
                                                    shade_size,
                                                    shade_period,
                                                    shade_duration,
                                                    shade_attenuation,
                                                    shade_offset,
                                                    lifetime))
        shaders.append(new_shade)
        print "New shader:", shade_size, shade_duration, shade_period, shade_offset, shaders[-1].attenuation
        print new_shade.history

    pickle.dump(shaders, open(pickle_file, 'w'))
