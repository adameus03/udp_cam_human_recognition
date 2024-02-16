import socket
import numpy as np
import cv2
import matplotlib.pyplot as plt


UDP_IP = "0.0.0.0"
UDP_PORT = 3333

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)#
sock.bind((UDP_IP, UDP_PORT))
s = ""

axesImage = None
axesImageSet = False
while True:
    print('listening')
    #data,addr = sock.recvfrom(2000000)
    #print('received')
    #data, addr = sock.recvfrom(46080)
    #s = s+data
    #frame = np.fromstring(s, dtype='uint8')

    s = b''
    nblks = 0

    aliveness_marked = False
    second_packet_received = False
    while True:
        #sock.listen(1)#
        #conn, addr = sock.accept()#
        #if aliveness_marked and not second_packet_received:
            #print('Waiting for second packet')
            
        #data, addr = sock.recvfrom(1024*3//2) #increase?
        data, addr = sock.recvfrom(32768)

        #if aliveness_marked and not second_packet_received:
            #print('Second packet received')
            #second_packet_received = True

        #if not aliveness_marked:
            #send 1 byte confirmation back to the client
            #sock.sendto(b'1', addr)
            #print('First packet received, sent aliveness confirmation')
            #aliveness_marked = True
        s += data
        nblks += 1
        if len(data) < 32768:
            print('received', nblks, 'blocks')
            break

    
    #print size of s
    print(len(s))
    #Assuming s contains jpeg bytes, decode it to a frame and display it
    frame = cv2.imdecode(np.frombuffer(s, dtype=np.uint8), cv2.IMREAD_COLOR)
    #Save the frame to a file
    #cv2.imwrite('frame.jpg', frame)
    # Display the resulting frame
    try:
        #cv2.imshow('frame', frame)
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
            #plt.pause(0.1)
    #Catch exceptions to avoid program crashing
    except Exception as e:
        print(e)
