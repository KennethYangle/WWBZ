#!/usr/bin/python
import cv2
import sys
import numpy as np

if len(sys.argv) != 2:
    print("Usage: {0} VIDEO_PATH".format(sys.argv[0]))
    sys.exit(-1)
if sys.argv[1] == "0":
    path = 0
else:
    path = sys.argv[1]
cap = cv2.VideoCapture(path)

frame_time = 1000/27
while(1):
    tic = cv2.getTickCount()
    ret, frame = cap.read()
    frame = cv2.resize(frame, (640, 480))

    hue_image = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
    low_range1 = np.array([170,0,230])
    high_range1 = np.array([179,100,255])
    th1 = cv2.inRange(hue_image, low_range1, high_range1)
    low_range2 = np.array([0,0,230])
    high_range2 = np.array([15,100,255])
    th2 = cv2.inRange(hue_image, low_range2, high_range2)
    th = th1 + th2
    # cv2.imshow("hsv", th)
    gray_image = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    edge = cv2.Canny(gray_image, 50, 500)
    cv2.imshow("edge", edge)
    # circles = cv2.HoughCircles(gray_image, cv2.HOUGH_GRADIENT, 4, 100, param1=100, param2=160, minRadius=0, maxRadius=0)

    circles = cv2.HoughCircles(th, cv2.HOUGH_GRADIENT, 4, 100, param1=100, param2=160, minRadius=0, maxRadius=0)
    cv2.waitKey(20)
    if circles is not None:
        circles = np.uint16(np.around(circles))
        obj = circles[0, 0]
        print(obj)
        cv2.circle(frame, (obj[0], obj[1]), obj[2], (0,255,0), 4)
        cv2.circle(frame, (obj[0], obj[1]), 2, (0,255,255), 3)
    cv2.imshow("capture", frame)
    toc = cv2.getTickCount()
    processing_time = (toc - tic) / cv2.getTickFrequency() * 1000
    print("Processing Time: {0} ms".format(processing_time))
    if processing_time < frame_time-1:
        # print(int(frame_time-processing_time))
        if cv2.waitKey(int(frame_time-processing_time)) & 0xFF == ord('q'):
            break
    else:
        cv2.waitKey(1)

cap.release()
cv2.destroyAllWindows()
