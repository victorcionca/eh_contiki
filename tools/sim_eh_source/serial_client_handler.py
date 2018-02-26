import socket
from threading import Thread
from time import sleep
import logging
logging.basicConfig(filename='eh_source.log', level=logging.DEBUG)

class SerialClientHandler(Thread):
    #def __init__(self, _socket, address, init_time, eh_trace):
    def __init__(self, position, port, init_time, e_manager, log_lock):
        """Handler for a serial client connection.

        Communicates with a node over the serial line, through
        the serial socket.
        Keeps track of the node's time, which can differ from real time.
        
        Params
        position    -- position of the node represented by this client
        _socket     -- communication handle for talking to the node
        init_time   -- time when the node booted up and connected.
        e_manager   -- energy manager that determines how much energy the node gets in an interval
        """
        Thread.__init__(self)
        self.time = init_time
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.position = position
        self.e_manager = e_manager
        self.period = 600       # hard-coded 10 minute period for EH update
        self.address = port
        self.log_lock = log_lock

    def run(self):
        # First try to connect (wait until connection succeeds)
        connected = False
        print "Connecting to port", self.address
        while not connected:
            try:
                self.sock.connect(('localhost', self.address))
                connected = True
            except:
                #print "Failed, trying again"
                sleep(1)
                
        print "Client", self.address, "is connected"
        # The message might come in one char at a time so
        # we have to parse input
        msg = '0000'
        while True:
            b = self.sock.recv(1)
            if len(b) == 0:
                break
            msg = msg[1:]+b
            if msg[:4] == 'E#H#':
                # ideally the period would come in the message TODO
                harvested = self.e_manager.get_energy(self.position,
                                                      self.time, self.time + self.period)
                self.log_lock.acquire()
                logging.debug("%d %s %0.2f" % (self.time, str(self.position), harvested))
                self.log_lock.release()
                self.time += self.period
                self.sock.send(str(int(harvested))+'\n')
        print "Serial client exiting"
