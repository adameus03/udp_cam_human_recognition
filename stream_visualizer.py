import socket
import cv2
import numpy as np
 

localIP     = "0.0.0.0"
localPort   = 3333
bufferSize  = 2000000

msgFromServer       = "Hello UDP Client"
bytesToSend         = str.encode(msgFromServer)

# Create a datagram socket
UDPServerSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

# Bind to address and ip
UDPServerSocket.bind((localIP, localPort))
print("UDP server up and listening")

def render_jpeg_from_message(message):
    print('rendering jpeg from message')
    try:
        # Decode message to jpeg
        #jpeg = cv2.imdecode(message, cv2.IMREAD_COLOR)
        jpeg = np.frombuffer(message, dtype=np.uint8)
        cv2.imshow('image', jpeg)
        cv2.waitKey(0)
        return jpeg
    except Exception as e:
        print('Error rendering jpeg from message', e)
    return None

# Listen for incoming datagrams
while(True):
    bytesAddressPair = UDPServerSocket.recvfrom(bufferSize)
    message = bytesAddressPair[0]
    address = bytesAddressPair[1]
    clientMsg = "Message from Client:{}".format(message)
    clientIP  = "Client IP Address:{}".format(address)
    print(clientMsg)
    print(clientIP)

    # Render jpeg from message
    jpeg = render_jpeg_from_message(message)

    # Sending a reply to client
    #UDPServerSocket.sendto(bytesToSend, address)