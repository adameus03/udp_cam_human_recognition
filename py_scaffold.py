import socket
import numpy as np
import cv2
import matplotlib.pyplot as plt


UDP_IP = "0.0.0.0"
UDP_PORT = 3333
MAX_UDP_DATA_SIZE = 65500

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)#
sock.bind((UDP_IP, UDP_PORT))
sock.setblocking(1) # set blocking

def check_jfif(data):
    #Check if the last chunk of data is the end of the image using rfind
    if data.rfind(b'\xff\xd9') != -1:
        # End of image found
        return True
    else:
        # End of image not found
        return False
    


def listen_udp():
    global sock
    print("Waiting for data")
    while True:
        data, addr = sock.recvfrom(MAX_UDP_DATA_SIZE)
        print('Received data of length ', len(data))
        #if check_jfif(data):
        #    print('Received jfif data of length ', len(data))
        #else:
        #    print('Received INVALID data of length ', len(data))


listen_udp()

    