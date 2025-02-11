import cv2
import mediapipe as mp
import os
import struct
import time
from string import Template
from multiprocessing.shared_memory import SharedMemory
import concurrent.futures

executor = concurrent.futures.ThreadPoolExecutor(max_workers=2)

shm = SharedMemory(create=True,size=41,name="handPositionData")




cap = cv2.VideoCapture(0)

mpHands = mp.solutions.hands
hands = mpHands.Hands(
    static_image_mode=False,
    max_num_hands=1,
    min_detection_confidence=0.5,
    min_tracking_confidence=0.5
)
mpDraw = mp.solutions.drawing_utils

pTime = 0
cTime = 0

def process_frame(frame):
    imgRGB = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    return hands.process(imgRGB)

while True:
    success, img = cap.read()
    img = cv2.flip(img, 1)
    future = executor.submit(process_frame,img)
    results = future.result()
    if not success:
        #unsuccesfull scan signal
        shm.buf[40] = struct.pack('B', 0)[0]
        continue
    elif results.multi_hand_landmarks:
        for handLms in results.multi_hand_landmarks:
            for id, lm in enumerate(handLms.landmark):
                h, w, c = img.shape
                cx, cy = lm.x , lm.y 
                #print(id, cx, cy)
                if id == 4:
                    packed_data = struct.pack('ff',cx,cy)    
                    shm.buf[0:8] = packed_data
                if id == 8:
                    packed_data = struct.pack('ff',cx,cy)    
                    shm.buf[8:16] = packed_data
                if id == 12:
                    packed_data = struct.pack('ff',cx,cy)    
                    shm.buf[16:24] = packed_data
                if id == 16:
                    packed_data = struct.pack('ff',cx,cy)    
                    shm.buf[24:32] = packed_data
                if id == 20:
                    packed_data = struct.pack('ff',cx,cy)    
                    shm.buf[32:40] = packed_data
                shm.buf[40] = struct.pack('B', 1)[0]
                

                
            mpDraw.draw_landmarks(img, handLms, mpHands.HAND_CONNECTIONS)

    cTime = time.time()
    fps = 1 / (cTime - pTime)
    pTime = cTime

    cv2.putText(img, str(int(fps)), (10, 70), cv2.FONT_HERSHEY_PLAIN, 3,
            (255, 0, 255), 3)
    cv2.circle(img,(320,240),10,(0,255,0))
    cv2.imshow("Image", img)
    key = cv2.waitKey(1) & 0xFF
    if key == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()