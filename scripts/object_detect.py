#!/usr/bin/python
import cv2
import sys
import numpy as np

def diagonal_check(p):
    d1 = np.sqrt(np.dot(p[0]-p[2], p[0]-p[2]))
    d2 = np.sqrt(np.dot(p[1]-p[3], p[1]-p[3]))
    return abs(d1-d2)*1.0/d1

def circleDetect(th):
    circles = cv2.HoughCircles(th, cv2.HOUGH_GRADIENT, 4, 100, param1=100, param2=160, minRadius=0, maxRadius=0)
    if circles is not None:
        circles = np.uint16(np.around(circles))
        obj = circles[0, 0]
        return obj
    else:
        return [-1, -1, -1]

def squareDetect(th):
    squares = []
    binary, cnts, hierarchy = cv2.findContours(th, cv2.RETR_LIST, cv2.CHAIN_APPROX_SIMPLE)
    for cnt in cnts:
        cnt_len = cv2.arcLength(cnt, True)
        cnt = cv2.approxPolyDP(cnt, 0.1 * cnt_len, True)
        if len(cnt) == 4 and cv2.contourArea(cnt) > 1000 and cv2.isContourConvex(cnt):
            cnt = cnt.reshape(-1, 2)
            diag_delta = diagonal_check(cnt)
            if diag_delta < 0.1:
                squares.append(cnt)
    return squares

if len(sys.argv) != 2:
    print("Usage: {0} VIDEO_PATH".format(sys.argv[0]))
    sys.exit(-1)
cap = cv2.VideoCapture(sys.argv[1])

frame_time = 1000/27
while(1):
    tic = cv2.getTickCount()
    ret, frame = cap.read()

    hue_image = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
    low_range1 = np.array([175,0,230])
    high_range1 = np.array([179,100,255])
    th1 = cv2.inRange(hue_image, low_range1, high_range1)
    low_range2 = np.array([0,0,230])
    high_range2 = np.array([15,100,255])
    th2 = cv2.inRange(hue_image, low_range2, high_range2)
    th = th1 + th2
    # cv2.imshow("hsv", th)

    obj = circleDetect(th)
    if obj[0] != -1:
        print(obj)
        cv2.circle(frame, (obj[0], obj[1]), obj[2], (0,255,0), 4)
        cv2.circle(frame, (obj[0], obj[1]), 2, (0,255,255), 3)
    
    # squares = squareDetect(th)
    # print(squares)
    # cv2.drawContours( frame, squares, -1, (255, 0, 0), 4)

    cv2.imshow("capture", frame)
    toc = cv2.getTickCount()
    processing_time = (toc - tic) / cv2.getTickFrequency() * 1000
    print("Processing Time: {0} ms".format(processing_time))
    if processing_time < frame_time-1:
        if cv2.waitKey(int(frame_time-processing_time)) & 0xFF == ord('q'):
            break
    else:
        cv2.waitKey(1)

cap.release()
cv2.destroyAllWindows()
