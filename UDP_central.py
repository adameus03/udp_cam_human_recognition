import socket
import numpy as np
import cv2
import matplotlib.pyplot as plt
from queue import PriorityQueue, SimpleQueue
import threading
from dataclasses import dataclass, field
from typing import Any
import os
import time
import math

#import pysine
#from pysinewave import SineWave

# start sinewave
#sinewave = SineWave(pitch_per_second = 100)
#sinewave.set_frequency(0)
#sinewave.play()

fps = 0
last_time = 0
frame_counter = 0


UDP_IP = "0.0.0.0"
#UDP_PORT = 3333
UDP_PORT = 8090
#MAX_UDP_DATA_SIZE = 65500
MAX_UDP_DATA_SIZE = 32750

jfif_chunks_queue = PriorityQueue()
jfif_queue = SimpleQueue()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)#
sock.bind((UDP_IP, UDP_PORT))
sock.setblocking(1) # set blocking

def basic_check_jfif(data):
    #Check if the last chunk of data is the end of the image using rfind
    if data.rfind(b'\xff\xd9') != -1:
        # End of image found
        return True
    else:
        # End of image not found
        return False
    

def parse_chunk(data):
    """
    First 4 bytes of `data` should contain the packet index
    Next 1 byte should be 0x1 for the first packet, 0x2 for the last packet, 0x3 for an only packet and 0x0 for all other packets
    All further bytes should be the actual JFIF chunk data
    """
    packet_index = int.from_bytes(data[0:4], byteorder='little')
    packet_type = int.from_bytes(data[4:5], byteorder='big')
    # CSID has 16 bytes, ignore it for this script as it is for test purpouses only
    print("CSID is " + str(data[5:21]))
    chunk_data = data[21:]
    #chunk_data = data[5:]
    
    return packet_index, packet_type, chunk_data
    

@dataclass(order=True)
class JfifChunkApplicationSegment:
    """
    The class is used as an item type in the `jfif_chunks_queue` priority queue
    """
    packet_index: int
    packet_type: Any=field(compare=False)
    chunk_data: Any=field(compare=False)

class Visualizer:
    def __init__(self, IMAGE_RENDER_DELAY_MS=10):
        self.axesImage = None
        self.axesImageSet = False
        self.IMAGE_RENDER_DELAY_MS = IMAGE_RENDER_DELAY_MS

    def image_update(self, jfif_data):
        global fps
        global last_time
        global frame_counter
        
        # handle fps calculation
        frame_counter += 1
        current_time = time.time()
        if current_time - last_time >= 1:
            fps = frame_counter
            frame_counter = 0
            last_time = current_time
            
        # store raw jfif_data bytes to a file
        #with open('frame_CENTRAL_raw.jpg', 'wb') as f:
        #    f.write(jfif_data)
            
        #frame = cv2.imdecode(np.from)
        frame = cv2.imdecode(np.frombuffer(jfif_data, dtype=np.uint8), cv2.IMREAD_COLOR)
        #print(type(frame))
        #cv2.imwrite('frame_CENTRAL.jpg', frame)
        #cv2.imwrite('live_frame.jpg', frame)
        """
        //https://stackoverflow.com/questions/66166929/send-jpeg-images-motion-jpeg-through-rtsp-gstreamer
        https://superuser.com/questions/1463858/streaming-jpg-stream-over-rtp-with-gstreamer-or-avconv  

        gst-launch-1.0 multifilesrc location="live_frame.jpg" loop=true start-index=0 stop-index=0  ! image/jpeg,width=640,height=512,type=video,framerate=30/1 ! identity ! jpegdec ! videoscale ! videoconvert ! x264enc ! h264parse ! mpegtsmux ! rtpmp2tpay ! udpsink host=127.0.0.1 port=5000
        """
        #return
        try:
            if not self.axesImageSet:
                plt.ion()
                fig = plt.figure()
                self.axesImage = plt.imshow(frame, aspect='auto')
                self.axesImageSet = True
                # print fps as title
                plt.title('FPS: ' + str(fps))

                #pysine.sine(frequency=440.0, duration=1.0) 
                ##sinewave.set_frequency(100 * fps)
                ##sinewave.play()

                plt.show()
                plt.pause(self.IMAGE_RENDER_DELAY_MS / 1000.0)
                ##sinewave.stop()
                
            else:
                self.axesImage.set_data(frame)
                # print fps as title
                plt.title('FPS: ' + str(fps))

                #pysine.sine(frequency=440.0, duration=1.0) 
                ##sinewave.set_frequency(100 * fps)
                ##sinewave.play()

                plt.draw()
                plt.pause(self.IMAGE_RENDER_DELAY_MS / 1000.0) # pause a bit so that plots are updated
                ##sinewave.stop()
        except Exception as e:
            print(e)


def named_pipe_send(pname="/tmp/f8a22310975600b1", data=b''):
    print("[Visualizer] START NAMED_PIPE_SEND")
    if data == b'':
        print("[Visualizer] Not sending empty data")
        return
    if not os.path.exists(pname):
        os.mkfifo(pname)
    with open(pname, 'wb') as f:
        f.write(data)

    print("[Visualizer] END NAMED_PIPE_SEND")

def run_tone_freq_updater():
    global fps
    global last_time
    global frame_counter
    ##global sinewave

    ##sinewave = SineWave(pitch_per_second = 1000)
    ##sinewave.set_frequency(0)
    ##sinewave.play()
    while True:
        ##sinewave.set_frequency(300 * math.log(fps + 1))
        # reset counter to 0 if needed
        current_time = time.time()
        if current_time - last_time >= 2.0:
            fps = 0
            frame_counter = 0
        time.sleep(0.1)

#If the reassembler fills the queue faster than the visualizer can consume it, the reassembler could block until the visualizer consumes some of the queue.
#But we don't want that, because the reassembler should be able to keep up with the incoming data.
#What is the solution???
#Maybe the reassembler should keep track of the queue size and if it exceeds a certain threshold, it should start dropping the oldest items from the queue.
#Otherwise, maybe using a different renderer that can keep up with the queue is the solution. 
#For example, replace the matplotlib visualizer with a simple cv2.imshow visualizer???
def run_visualizer():
    """
    This is the consumer of the `run_reassembler` function
    """
    visualizer = Visualizer(IMAGE_RENDER_DELAY_MS=0.1)
    while True:
        jfif_data = jfif_queue.get(block=True)
        print("[Visulizer] Received JFIF data to visualize")
        if basic_check_jfif(jfif_data):
            print("[Visualizer] basic_jfif_check OK")
            #print(type(jfif_data))
            
            #print("Before image_update")
            numQueued = jfif_queue.qsize()
            print("[Visualizer] numQueued=" + str(numQueued))#"qsize is not reliable"
            if numQueued > 1:
                numJfifsToDrop = numQueued - 1
                print("[Visualizer] Dropping " + str(numJfifsToDrop) + " JFIFs")
                for i in range(numJfifsToDrop):
                    jfif_queue.get(block=False)
                    
            visualizer.image_update(jfif_data)
            #named_pipe_send(data=jfif_data)
            #print("After image_update")
        else:
            print("[Visualizer] basic_jfif_check FAIL")

def run_reassembler(patience=5):
    """
    When the reassembler finds the starting chunk, it will try to find the ending chunk which is no further away than the number indicated by `patience`
    This is the consumer of the `listen_udp` function
    This is the producer for the `run_visualizer` function  
    """
    global jfif_chunks_queue
    global jfif_queue

    premature_next_multichunk_jfif_encountered = False
    premature_data = b''
    
    while True:
        jfif_data = b''
        # Scanning queued chunks for single JFIF instance reassembly

        premature_only_jfif_encountered = False

        starting_chunk_found = False
        last_chunk_found = False
        only_chunk_found = False
        corrupted_metadata_found = False

        is_chunk_flow_continuous = True
        chunk_control_index = -1
        if premature_next_multichunk_jfif_encountered:
            jfif_data = premature_data
            premature_next_multichunk_jfif_encountered = False
            starting_chunk_found = True
        else:
            while True:
                chunk = jfif_chunks_queue.get(block=True) #block until queue is non-empty
                if chunk.packet_type == 1:
                    starting_chunk_found = True
                    jfif_data += chunk.chunk_data
                    chunk_control_index = chunk.packet_index
                    break
                elif chunk.packet_type == 3:
                    only_chunk_found = True
                    jfif_data += chunk.chunk_data
                    break
                elif chunk.packet_type == 2:
                    print("[Reassembler] Out of context last chunk marker encountered while scanning for starting/only JFIF chunk. Continuing the scan.")
                elif chunk.packet_type != 0:
                    print("[Reassembler] Chunk with corrupted metadata found while scanning for starting/only JFIF chunk. Continuing the scan.")


        if starting_chunk_found:
            print("[Reassembler] Found starting JFIF chunk. Now scanning for end chunk")
        
        elif only_chunk_found:
            print("[Reassembler] Found the only JFIF chunk")
            jfif_queue.put(jfif_data, block=False) #
            continue

        for patience_counter in range(patience):
            chunk = jfif_chunks_queue.get(block=True)

            if chunk.packet_index != chunk_control_index + 1:
                is_chunk_flow_continuous = False
            chunk_control_index = chunk.packet_index

            if chunk.packet_type == 1:
                #starting_chunk_found = True
                print("[Reassembler] Premature starting chunk marker encountered while scanning for end JFIF chunk. Skipping to the next JFIF.")
                premature_next_multichunk_jfif_encountered = True
                premature_data = chunk.chunk_data
                break
            elif chunk.packet_type == 2:
                jfif_data += chunk.chunk_data
                last_chunk_found = True
                print("[Reassembler] Found the last JFIF chunk")
                break
            elif chunk.packet_type == 3:
                #only_chunk_found = True
                print("[Reassembler] Premature only chunk marker encountered while scanning for end JFIF chunk. Sending it to the jfif queue and moving to the next JFIF.")
                premature_only_jfif_encountered = True
                jfif_queue.put(chunk.chunk_data, block=False)
                break
            elif chunk.packet_type == 0: #intermediate chunk
                jfif_data += chunk.chunk_data
            else:
                corrupted_metadata_found = True
                #break

        if premature_only_jfif_encountered:
            continue
        elif premature_next_multichunk_jfif_encountered:
            continue
        elif not is_chunk_flow_continuous:
            print("[Reassembler] Lost some chunks of this JFIF. Skipping to the next one.")
            continue
        elif corrupted_metadata_found:
            # Is this right, or maybe send it to the queue anyways?
            print("[Reassembler] Found some chunk(s) with corrupted metadata. Skipping to the next JFIF.")
        elif not last_chunk_found:
            print("[Reassembler] Patience exceeded while scanning for ending JFIF chunk. Skipping to the next JFIF threating making any remaining data to orphans.")
        else: # last chunk found
            jfif_queue.put(jfif_data, block=False)
        
        
            



def listen_udp():
    """
    This is the producer for the `run_reassembler` function
    """
    global sock
    global jfif_chunks_queue
    print("[Listener] Waiting for data")
    while True:
        data, addr = sock.recvfrom(MAX_UDP_DATA_SIZE)
        packet_index, packet_type, chunk_data = parse_chunk(data)
        print('[Listener] Received packet ', packet_index, ' of type ', packet_type, ' with length ', len(chunk_data))
        jfif_chunks_queue.put(JfifChunkApplicationSegment(packet_index, packet_type, chunk_data), block=False)
        #if check_jfif(data):
        #    print('Received jfif data of length ', len(data))
        #else:
        #    print('Received INVALID data of length ', len(data))


listener_thread = threading.Thread(target=listen_udp)
reassembler_thread = threading.Thread(target=run_reassembler, args=(10,))
#tone_updater_thread = threading.Thread(target=run_tone_freq_updater)
#visualizer_thread = threading.Thread(target=run_visualizer)

#visualizer_thread.start()
#print("Visualizer thread started")
reassembler_thread.start()
print("Reassembler thread started")
listener_thread.start()
print("Listener thread started")

#tone_updater_thread.start()
#print("Tone updater thread started")

run_visualizer()

"""
    @warning Should not reach here for now anyways unless the user kills the process
"""
listener_thread.join()
print("Visualizer thread finished")
reassembler_thread.join()
print("Reassembler thread finished")
#visualizer_thread.join()
#print("Visualizer thread finished")




