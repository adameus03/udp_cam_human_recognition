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

    quick_fast_forward = False
    data = None #
    addr = None #
    while True:
        if quick_fast_forward:
            sock.sendto(b'', addr)
            quick_fast_forward = False
        else:
            data, addr = sock.recvfrom(0)
            sock.sendto(b'', addr)

        s = b''
        nblks = 0
        while True:
            #clear the buffer
            data, addr = sock.recvfrom(32768)
            s += data
            nblks += 1
            if len(data) == 0:
                print("quick fast forward")
                quick_fast_forward = True
                break
            elif len(data) < 32768:
                print('received', nblks, 'blocks')
                break
            elif s.rfind(b'\xff\xd9') != -1:
                print("Eureka")
                print('received', nblks, 'blocks')

        if quick_fast_forward:
            continue
        if check_jpeg(s, data):
            image_update(s)
        # reset the socket
        #sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        #sock.bind((UDP_IP, UDP_PORT))

listen_udp()