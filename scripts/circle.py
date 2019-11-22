import sys
sys.path.remove('/opt/ros/kinetic/lib/python2.7/dist-packages')
import traceback
import tellopy
import av
import cv2
import time
import mmap
import multiprocessing
import datetime
import numpy as np


"""mission.value:\n
    -1  takeoff
    0   idle
    1   search1
    2   approach1
    3   through1
    4   s2      5   a2      6   t2
    7   s3      8   a3      9   t3
    10  land
    """


def handler(event, sender, data, **args):
    drone = sender
    if event is drone.EVENT_FLIGHT_DATA:
        print(data)

def saturation(x):
    if x > 1.0:
        x = 1.0
    elif x < -1.0:
        x = -1.0
    return x

def createGammaTable(gamma=1.0):
    # build a lookup table mapping the pixel values [0, 255] to their adjusted gamma values
    table = np.array([((i / 255.0) ** gamma) * 255
                      for i in np.arange(0, 256)]).astype("uint8")
    return table

def dilate_color(image_hue, color):
    if color == "green":
        green_low = np.array([65, 60, 215])
        green_high = np.array([86, 130, 256])
        th = cv2.inRange(image_hue, green_low, green_high)
    
    elif color == "blue":
        blue_low = np.array([85, 50, 220])
        blue_high = np.array([105, 100, 256])
        th = cv2.inRange(image_hue, blue_low, blue_high)

    elif color == "red":
        red_low_1 = np.array([170, 150, 230])
        red_high_1 = np.array([180, 256, 256])
        th1 = cv2.inRange(image_hue, red_low_1, red_high_1)
        red_low_2 = np.array([0, 150, 230])
        red_high_2 = np.array([20, 256, 256])
        th2 = cv2.inRange(image_hue, red_low_2, red_high_2)
        th = th1 + th2

    else:
        print("unknow color!!!!!")
        return

    dilated = cv2.dilate(th, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3)), iterations=2)

    cv2.imshow("dilated", dilated)
    cv2.waitKey(1)
    return dilated


def find_circle(dilated):
    circles = cv2.HoughCircles(dilated, cv2.HOUGH_GRADIENT, 4, 100, param1=100, param2=160, minRadius=0, maxRadius=0)
    if circles is not None:
        circles = np.uint16(np.around(circles))
        obj = circles[0, 0]
        return obj[0], obj[1], obj[2]
    else:
        return -1, -1, -1

def find_square(dilated):
    circles = cv2.HoughCircles(dilated, cv2.HOUGH_GRADIENT, 4, 100, param1=100, param2=160, minRadius=0, maxRadius=0)
    if circles is not None:
        circles = np.uint16(np.around(circles))
        obj = circles[0, 0]
        return obj[0], obj[1], obj[2]
    else:
        return -1, -1, -1

def calc_centroid(dilated):
    min_prop = 0.001

    M = cv2.moments(dilated, binaryImage = True)
    if M["m00"] >= min_prop*w*h:
        cx = int(M["m10"] / M["m00"])
        cy = int(M["m01"] / M["m00"])
        return cx, cy

    else:
        return -1, -1


def droneSetup(mission, ox, oy):
    """Subprocess.\n
    Takeoff drone and write image to mmap."""
    # 0. get tello object
    drone = tellopy.Tello()
    # 0.1. parameters
    k_yaw = 0.004
    k_z = -0.005
    attack_pitch = 0.6
    servo_centre_x = 0.5
    servo_centre_y = 0.4
    search_v = 0.2
    search_dir = [-1, 
                1, 1, 1,         # s1 direction
                -1, -1, -1,         # s2
                -1, -1, -1, -1]     # s3

    # 1. connect
    try:
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
            # drone.up(50)
            # time.sleep(2)
        except Exception as ex:
            print(ex)

        # 3. get image and write
        while True:
        # for i in range(1000):
            for frame in container.decode(video=0):
                # get image
                image = cv2.cvtColor(np.array(frame.to_image()), cv2.COLOR_RGB2BGR)
                # write image share
                image_share = image.tostring()
                m.seek(0)
                m.write(image_share)
                m.flush()

                # 4. execute mission
                print("mission:{}".format(mission.value))
                # search
                if int(mission.value) == 0:
                    drone.set_yaw(0.0)
                    drone.set_throttle(0.0)
                    drone.set_pitch(0.0)
                    # time.sleep(0.1)

                elif int(mission.value) == -1:
                    drone.takeoff()
                    drone.set_yaw(0.0)
                    drone.set_throttle(0.0)
                    drone.set_pitch(0.0)
                    time.sleep(2)
                    mission.value = 1

                elif int(mission.value) == 1:
                    drone.set_yaw(0.0)
                    drone.set_throttle(0.1)
                    drone.set_pitch(0.0)

                elif int(mission.value) in [4, 7]:
                    drone.set_yaw(search_v * search_dir[mission.value])
                    drone.set_throttle(0.0)
                    drone.set_pitch(0.0)

                # approach
                elif int(mission.value) in [2, 5, 8]:
                    # get image size
                    h,w,l = np.shape(image)
                    # calc error on image
                    print(ox.value, oy.value, mission.value)
                    if ox.value < 0:
                        continue
                    err_x = ox.value - w * servo_centre_x
                    err_y = oy.value - h * servo_centre_y

                    drone.set_yaw(saturation(k_yaw * err_x))
                    drone.set_throttle(saturation(k_z * err_y))
                    drone.set_pitch(attack_pitch)
                    # drone.set_pitch(0)

                # through
                elif int(mission.value) in [3, 6, 9]:
                    # get image size
                    h,w,l = np.shape(image)

                    if ox.value > 0 and oy.value > 0:
                        # calc error on image
                        print(ox.value, oy.value, mission.value)
                        if ox.value < 0:
                            continue
                        err_x = ox.value - w * servo_centre_x
                        err_y = oy.value - h * servo_centre_y

                        drone.set_yaw(saturation(k_yaw * err_x))
                        drone.set_throttle(saturation(k_z * err_y))
                        drone.set_pitch(attack_pitch)
                        # drone.set_pitch(0)

                    else:
                        drone.set_yaw(0.0)
                        drone.set_throttle(0.0)
                        drone.set_pitch(attack_pitch)

                # land
                elif int(mission.value) == 10:
                    drone.land()

                else:
                    print(mission.value)
                    print("unknow mission!!!")


                # 5. esc
                key = cv2.waitKey(1)
                if key == 27:
                    drone.land()
                    drone.quit()
                    cv2.destroyAllWindows()

    except Exception as ex:
        exc_type, exc_value, exc_traceback = sys.exc_info()
        traceback.print_exception(exc_type, exc_value, exc_traceback)
        print(ex)
    finally:
        drone.land()
        drone.quit()
        cv2.destroyAllWindows()
        # cap_out.release()


if __name__ == '__main__':
    # 0.0. shared memory and shared value
    image_length = 960*720*3
    m = mmap.mmap(-1, image_length)

    mission = multiprocessing.Value('b', 0)
    ox = multiprocessing.Value('d', 0.0)
    oy = multiprocessing.Value('d', 0.0)

    # 0.1. define the storage path and create VideoWriter object
    now = datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
    path = './capture/' + now
    fourcc = cv2.VideoWriter_fourcc(*'XVID')
    cap_out = cv2.VideoWriter(path+'.avi', fourcc, 20.0, (960,720))
    cap_out_dilated = cv2.VideoWriter(path+'dilated.avi', cv2.VideoWriter_fourcc(*'XVID'), 20.0, (960,720))

    # 0.2. parameters
    mission_color = {-1:"green", 0:"green", 1:"green", 2:"green", 3:"green", 
                     4:"blue", 5:"blue", 6:"blue",
                     7:"red", 8:"red", 9:"red"}

    # 0.3. start drone process
    p = multiprocessing.Process(target=droneSetup, args=(mission,ox,oy))
    p.start()
    print("Sub-process start.")

    lookUpTable = createGammaTable(gamma=5.0)
    mission.value = 0
    frame_cnt = 0
    idle_cnt = 0
    through_cnt = 0
    while True:
        # 1.0. get image from mmap
        m.seek(0)
        image_share = m.read(image_length)
        image = np.fromstring(image_share, dtype=np.uint8).reshape(720, 960, 3)
        frame_cnt = frame_cnt + 1

        # # 1.1. skip first 300 frame
        # if frame_cnt < 100:
        #     time.sleep(0.05)
        #     continue

        # 1.2. resize image to speed up
        # image = cv2.resize(image, (480,360))
        h,w,l = np.shape(image)

        # 1.3. gamma transform and hsv convert
        image_gamma = cv2.LUT(image, lookUpTable)
        image_hue = cv2.cvtColor(image_gamma, cv2.COLOR_BGR2HSV)

        # 1.4. get dilated image
        color = mission_color[mission.value]
        print(color)

        dilated = dilate_color(image_hue, color)
        cap_out_dilated.write(dilated)

        # 2. state machine
        # takeoff and idle
        if mission.value == 0:
            idle_cnt += 1
            # time.sleep(0.1)
            if idle_cnt > 1000:
                mission.value = -1

        # search
        elif int(mission.value) in [1, 4, 7]:
            if color == "green" or color == "blue":
                cx, cy, R = find_circle(dilated)
            else:
                cx, cy, R = find_square(dilated)
            
            if cx > 0 and cy > 0:
                ox.value = cx
                oy.value = cy
                mission.value += 1

                cv2.circle(image_gamma, (cx, cy), R, (0,255,0), 4)
                cv2.circle(image_gamma, (cx, cy), 2, (0,255,255), 3)

            else:
                cx, cy = calc_centroid(dilated)
                if (cx > w / 3 and cx < w * 2 / 3) \
                    or (cy > h / 3 and cy < h * 2 / 3):
                    ox.value = cx
                    oy.value = cy
                    mission.value += 1

                    cv2.circle(image_gamma, (cx, cy), 2, (0,255,255), 3)

                else:
                    continue


        # approach
        elif int(mission.value) in [2, 5, 8]:
            if color == "green" or color == "blue":
                cx, cy, R = find_circle(dilated)
            else:
                cx, cy, R = find_square(dilated)
            print("R: {}".format(R))

            if cx > 0 and cy > 0:
                ox.value = cx
                oy.value = cy
                if R > 300:
                    mission.value += 1
                cv2.circle(image_gamma, (cx, cy), R, (0,255,0), 4)
                cv2.circle(image_gamma, (cx, cy), 2, (0,255,255), 3)

            else:
                cx, cy = calc_centroid(dilated)
                ox.value = cx
                oy.value = cy

                cv2.circle(image_gamma, (cx, cy), 2, (0,255,255), 3)


        # through
        elif int(mission.value) in [3, 6, 9]:
            if color == "green" or color == "blue":
                cx, cy, R = find_circle(dilated)
            else:
                cx, cy, R = find_square(dilated)
            
            if cx > 0 and cy > 0:
                through_cnt = 0
                ox.value = cx
                oy.value = cy
                cv2.circle(image_gamma, (cx, cy), R, (0,255,0), 4)
                cv2.circle(image_gamma, (cx, cy), 2, (0,255,255), 3)

            else:
                ox.value = -1
                oy.value = -1

                cx, cy = calc_centroid(dilated)
                if cx < 0 and cy < 0:
                    through_cnt += 1
                    if through_cnt > 10:
                        mission.value += 1
                        through_cnt = 0
                else:
                    through_cnt = 0

        elif mission.value == -1:
            print("waitting for takeoff.")

        else:
            print("Finish")



        # 2. write and show
        cv2.imshow("gamma", image_gamma)
        cap_out.write(image_gamma)
        cv2.waitKey(1)
