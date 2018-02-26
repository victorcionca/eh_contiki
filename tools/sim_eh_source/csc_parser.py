"""
Based on a Cooja csc file extracts the locations and IDs of the nodes.
"""

def parse_csc(csc_file):
    """
    Returns a list of tuples: (mote_id, position)
    """
    net = []
    with open(csc_file) as f:
        states = ['findmote', 'inmote']
        state = 'findmote'
        mote = None
        for line in f:
            if '</simulation>' in line: break
            if state == 'findmote':
                if '<mote>' in line:
                    state = 'inmote'
                    mote = {'id':None, 'x': None, 'y': None}
                    continue
            if state == 'inmote':
                if '<x>' in line:
                    mote['x'] = int(float(line.split('>')[1].split('<')[0]))
                    continue
                if '<y>' in line:
                    mote['y'] = int(float(line.split('>')[1].split('<')[0]))
                    continue
                if '<id>' in line:
                    mote['id'] = int(float(line.split('>')[1].split('<')[0]))
                    continue
                if '</mote>' in line:
                    net.append((mote['id'], (mote['x'], mote['y'])))
                    state = 'findmote'
    return net
