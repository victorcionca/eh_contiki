"""
This application listens for incoming connections from serial servers.

For each server it spawns a handler that will communicate through that
line with the node and provide EH information.
"""

import socket
from random import random, randint
from serial_client_handler import SerialClientHandler
from eh_trace import EHTrace
from csc_parser import parse_csc
from shader import ShadingPattern
from position import Rectangle
from energy_manager import EnergyManager


usage = 'python eh_source_server.py <base_port> <trace_file> <trace_offset> <csc_file> <shader.pickle>'

import sys
if len(sys.argv) < 4:
    print usage
    sys.exit(1)

try:
    port_offset = int(sys.argv[1])
except:
    print 'Second argument (the base port) must be an int between 1000 and 65535'
    print usage
    sys.exit(1)

try:
    day_offset = int(sys.argv[3])
except:
    print 'Third argument (trace offset) must be an integer'
    print usage
    sys.exit(1)

trace_file = sys.argv[2]
# import the energy harvesting trace
try:
    trace = EHTrace(trace_file, 30, wrap=True, start_sample=day_offset)
except:
    print 'Problems accessing the trace file', trace_file
    print usage
    sys.exit(1)

try:
    csc_file = sys.argv[4]
    net = parse_csc(csc_file)
except:
    print 'Problems accessing the csc file', csc_file
    print usage
    sys.exit(1)

if len(sys.argv) > 5:
    try:
        shaders_input_f = open(sys.argv[5])
        import pickle
        shaders_input = pickle.load(shaders_input_f)
    except:
        print 'Problems accessing the shaders file', sys.argv[5]
        print usage
        sys.exit(1)
else:
    shaders_input = None


#############
# What is the deployment area?
netcoords = zip(*net)[1]
depl_orig_x = min(zip(*netcoords)[0])
depl_orig_y = min(zip(*netcoords)[1])
width = max(zip(*netcoords)[0]) - depl_orig_x
height = max(zip(*netcoords)[1]) - depl_orig_y
depl_area = Rectangle((depl_orig_x, depl_orig_y),
                      width, height)

if shaders_input is None:
    # Generate the shading patterns
    size = min(width, height)
    shaders = []
    for s in xrange(2):
        shade_size = (0.1+random()*0.3)*size      # Shade size is \in [5%,10%] of area min size
        shade_duration = int(random()*5*3600)       # Shade length is less than 5 hours
        shade_period = [6,24][randint(0,1)]*3600          # Period is either 6 (twice daily) or 24 hours (daily)
        shade_offset = (6 + randint(0,4))*3600              # EHTrace starts at midnight, so offset by at least 6h
        shaders.append(ShadingPattern(
                                depl_area,
                                shade_size,
                                shade_period,
                                shade_duration,
                                shade_offset,
                                random()))
        print "New shader:", shade_size, shade_duration, shade_period, shade_offset, shaders[-1].attenuation
else:
    shaders = shaders_input

# Instantiate the energy manager
energy_man = EnergyManager(trace, shaders)

# Lock for synchronising access to the logfile
from threading import Lock
log_lock = Lock()

# Assume that sink is node 1
for i, n in enumerate(net):
    client = SerialClientHandler(n[1], port_offset + i, 0, energy_man, log_lock)
    print "Spawning client"
    client.start()
