class EHTrace:
    """Represents an EH timeseries, in power (W) vs time (s).
    Internally this is stored as an array, with a power element
    for each sampling period, so that it is assumed the power
    is constant between readings.
    """

    def __init__(self, trace_file, period, cutoff=None, wrap=False, start_sample=0):
        """Instantiates based on a file where irradiance readings are taken
        periodically. Each line represents a reading, with irradiance in uW/cm^2

        The time of the readings doesn't count, as long as the readings
        are sequential (no skipped samples). Therefore the file only
        needs to contain the readings.

        Params
        trace_file      -- the file holding the readings
        period          -- the interval between readings (lines).
        cutoff          -- instant where trace is cut off
        wrap            -- True to wrap back to start after cut off
        start_sample    -- index in trace where to start
        """
        self.trace = []
        self.period = period
        self.cutoff = cutoff
        self.wrap = wrap
        self.panel_area = 729 # area of the solar panel, to convert irradiance into power
        with open(trace_file) as f:
            l_num = 0
            for line in f:
                if cutoff and l_num*period >= cutoff:
                    break
                self.trace.append(int(float(line.split(',')[1][:-1])))
                l_num += 1
            if cutoff == None:
                self.cutoff = l_num*period

        self.trace = self.trace[start_sample:]

    def between(self, _from, to):
        """Determines the total energy between @from and @to.
        Considers wrap and cutoff.

        Returns: energy in Watt-ticks
        """
        # determine the index corresponding to from
        if _from >= self.cutoff:
            if self.wrap:
                _from = _from % self.cutoff
            else:
                return None     # tried to access beyond trace limit
        if to >= self.cutoff:
            if self.wrap:
                to = to % self.cutoff
            else:
                to = self.cutoff    # cut off at end of trace
        if _from > to:  # this could happen if @to has wrapped but _from hasn't
            return self.between_nowrap(_from, self.cutoff-1) +\
                   self.between_nowrap(0, to)
        else:
            return self.between_nowrap(_from, to)

    def between_nowrap(self, _from, to):
        """Determines the total energy between @from and @to.
        Doesn't consider wrap

        Returns: energy in Watt-ticks
        """
        # determine the index corresponding to from
        from_idx = _from/self.period
        to_idx = to/self.period
        if to_idx == len(self.trace):
            to_idx -= 1
        energy_joules = None
        if from_idx == to_idx:
            energy_joules = self.trace[from_idx] * (to - _from)
        else:
            complete_periods = self.trace[from_idx:to_idx]
            start = self.trace[from_idx]*(self.period-(_from%self.period))
            mid = sum([p*self.period for p in complete_periods])
            end = self.trace[to_idx]*(to % self.period)
            energy_joules = start + mid + end
        energy_joules *= self.panel_area
        return energy_joules*32768/1000000  # 32768 ticks per second
