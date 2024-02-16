import socket
import numpy as np
import cv2
import matplotlib.pyplot as plt


UDP_IP = "0.0.0.0"
UDP_PORT = 3333

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)#
sock.bind((UDP_IP, UDP_PORT))
# set blocking
sock.setblocking(1)

axesImage = None
axesImageSet = False

def check_jpeg(s, last_chunk):
    #Check if the last chunk of data is the end of the image using rfind
    if s.rfind(b'\xff\xd9') != -1:
        print("End of image found")
        return True
    else:
        print("End of image NOT found")
        print("Size of last chunk: ", len(last_chunk))
        return False

def image_update(s):
    global axesImageSet
    global axesImage
    frame = cv2.imdecode(np.frombuffer(s, dtype=np.uint8), cv2.IMREAD_COLOR)
    try:
        if not axesImageSet:
            plt.ion()
            fig = plt.figure()
            axesImage = plt.imshow(frame, aspect='auto')
            axesImageSet = True
            plt.show()
            plt.pause(10)
            
        else:
            axesImage.set_data(frame)
            plt.draw()
            plt.pause(0.1) # pause a bit so that plots are updated
    except Exception as e:
        print(e)

def listen_udp():
    global sock

    num_next_jpegs_awaiting = 0
    skipping = False
    data = None #
    addr = None #

    while True:
        if num_next_jpegs_awaiting > 0:
            sock.sendto(b'', addr)
        else:
            print('waiting for decoy packet')
            data, addr = sock.recvfrom(32678)
            while len(data) != 0:
                print('not a decoy packet')
                data, addr = sock.recvfrom(32678)
            
            sock.sendto(b'', addr)
            print('decoy packet received, sent confirmation back to client')

        s = b''
        nblks = 0
        while True:
            data, addr = sock.recvfrom(32768)
            s += data
            nblks += 1
            if len(data) == 0:
                num_next_jpegs_awaiting += 1
                print("Num next jpegs awaiting: ", num_next_jpegs_awaiting)
                if (num_next_jpegs_awaiting > 2):
                    print("skipping to newest jpeg")
                    num_next_jpegs_awaiting = 0
                    skipping = True
                    break
            elif len(data) < 32768:
                print('received', nblks, 'blocks')
                break
            elif s.rfind(b'\xff\xd9') != -1:
                print("Eureka")
                print('received', nblks, 'blocks')
            else:
                print('actual blk received')

        if not skipping:
            #if num_next_jpegs_awaiting > 0:
            #    num_next_jpegs_awaiting -= 1
            #    if num_next_jpegs_awaiting == 0:
            #        print("skipping complete")
            #        skipping = False
            #    continue
            if num_next_jpegs_awaiting > 0:
                num_next_jpegs_awaiting -= 1
            #if check_jpeg(s, data):
            #    image_update(s)
        else:
            skipping = False
        
        # reset the socket
        #sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        #sock.bind((UDP_IP, UDP_PORT))

listen_udp()