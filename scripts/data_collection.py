import sys
sys.path.remove('/opt/ros/kinetic/lib/python2.7/dist-packages')
import tellopy
import av
import cv2
import time
import datetime
import numpy as np

def handler(event, sender, data, **args):
    drone = sender
    if event is drone.EVENT_FLIGHT_DATA:
        print(data)

def createGammaTable(gamma=1.0):
    # build a lookup table mapping the pixel values [0, 255] to their adjusted gamma values
    table = np.array([((i / 255.0) ** gamma) * 255
                      for i in np.arange(0, 256)]).astype("uint8")
    return table

def main():
    lookUpTable = createGammaTable(gamma=5.0)

    # 0. get tello object, get video writer
    drone = tellopy.Tello()
    now = datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
    path = './capture/' + now
    fourcc = cv2.VideoWriter_fourcc(*'XVID')
    cap_out = cv2.VideoWriter(path+'_collection.avi', fourcc, 20.0, (960,720))

    drone.connect()
    drone.wait_for_connection(60.0)

    retry = 3
    container = None
    while container is None and 0 < retry:
        retry -= 1
        try:
            container = av.open(drone.get_video_stream())
        except av.AVError as ave:
            print(ave)
            print('retry...')

    # 2. takeoff
    try:
        drone.subscribe(drone.EVENT_FLIGHT_DATA, handler)
        # drone.takeoff()
    except Exception as ex:
        print(ex)
    # skip first 300 frames
    frame_skip = 300
    for i in range(150):
        for frame in container.decode(video=0):
            # 3. get picture
            if 0 < frame_skip:
                frame_skip = frame_skip - 1
                continue
            image = cv2.cvtColor(np.array(frame.to_image()), cv2.COLOR_RGB2BGR)
            image_gamma = cv2.LUT(image, lookUpTable)
            cv2.imshow('Original', image_gamma)
            cap_out.write(image_gamma)
            cv2.waitKey(1)

    drone.land()


if __name__ == '__main__':
    main()
