import cv2
import mediapipe as mp
import struct
import time
from multiprocessing import Process
from multiprocessing.shared_memory import SharedMemory

def track_hand0( shm_name, offset):
    cap = cv2.VideoCapture(0)
    hands = mp.solutions.hands.Hands(
        static_image_mode=False,
        max_num_hands=1,
        min_detection_confidence=0.5,
        min_tracking_confidence=0.5
    )
    mpDraw = mp.solutions.drawing_utils

    shm = SharedMemory(name=shm_name)
    
    while True:
        success, img = cap.read()
        if not success:
            shm.buf[offset + 40] = struct.pack('B', 0)[0]  # Failed signal
            continue
        
        img = cv2.flip(img, 1)
        imgRGB = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        results = hands.process(imgRGB)

        if results.multi_hand_landmarks:
            for handLms in results.multi_hand_landmarks:
                for id, lm in enumerate(handLms.landmark):
                    if id in [4, 8, 12, 16, 20]:  # Thumb, Index, Middle, Ring, Pinky
                        index = offset + (id - 4) // 4 * 8
                        packed_data = struct.pack('ff', lm.x, lm.y)
                        shm.buf[index:index+8] = packed_data
                    shm.buf[offset + 40] = struct.pack('B', 1)[0]
                mpDraw.draw_landmarks(img, handLms, mp.solutions.hands.HAND_CONNECTIONS)

        cv2.imshow(f"Camera {0}", img)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()
    shm.close()

def track_hand1( shm_name, offset):
    cap = cv2.VideoCapture(1)
    hands = mp.solutions.hands.Hands(
        static_image_mode=False,
        max_num_hands=1,
        min_detection_confidence=0.5,
        min_tracking_confidence=0.5
    )
    mpDraw = mp.solutions.drawing_utils

    shm = SharedMemory(name=shm_name)
    
    while True:
        success, img = cap.read()
        if not success:
            shm.buf[offset + 20] = struct.pack('B', 0)[0]  # Failed signal
            continue
        
        img = cv2.flip(img, 1)
        imgRGB = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        results = hands.process(imgRGB)

        if results.multi_hand_landmarks:
            for handLms in results.multi_hand_landmarks:
                for id, lm in enumerate(handLms.landmark):
                    if id in [4, 8, 12, 16, 20]:  # Thumb, Index, Middle, Ring, Pinky
                        index = offset + (id - 4) // 4 * 8
                        packed_data = struct.pack('f', lm.y)
                        shm.buf[index:index+4] = packed_data
                    shm.buf[offset + 20] = struct.pack('B', 1)[0]
                mpDraw.draw_landmarks(img, handLms, mp.solutions.hands.HAND_CONNECTIONS)

        cv2.imshow(f"Camera {1}", img)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()
    shm.close()

if __name__ == "__main__":
    shm = SharedMemory(create=True, size=62, name="handPositionData")  # 1x41 and 1x21 for two cameras

    process1 = Process(target=track_hand0, args=("handPositionData", 0))
    process2 = Process(target=track_hand1, args=("handPositionData", 41))

    process1.start()
    process2.start()

    process1.join()
    process2.join()
    shm.unlink()
    