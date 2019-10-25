#include <iostream>
#include <fstream>
#include <sstream>
#include <opencv2/opencv.hpp>
#include "rapidjson/document.h"
#include "share_memory.h"

using namespace cv;
using namespace std;

#define expand_thre 3 * CV_PI / 180
#define rho_thre 50
#define INF 0x3f3f3f3f

// changing contrast brightness gamma correction
Mat lookUpTable(1, 256, CV_8U);
void createGammaTable(const double gamma_)
{
    uchar* p = lookUpTable.ptr();
    for( int i = 0; i < 256; ++i)
        p[i] = saturate_cast<uchar>(pow(i / 255.0, gamma_) * 255.0);
}


int main(int argc, char **argv)
{
    // parse args
    if (argc > 2)
    {
        printf("Usage: %s [CONFIG_FILE]\n", argv[0]);
        return -1;
    }
    string cfg_path = "./cfg/config.json";
    if (argc == 2)
    {
        cfg_path = argv[1];
    }

    // read config file and get video path
    ifstream in(cfg_path);
    ostringstream tmp;
    tmp << in.rdbuf();
    string json = tmp.str();
    cout << "cfg:\n" << json << endl;

    rapidjson::Document document;
    document.Parse(json.c_str());
    string video_path = document["test"]["video_path"].GetString(); // "./video/20160212_003550.avi"

    // get video capture
    VideoCapture cap;
    if (video_path == "0")
    {
        cap.open(0);
        cout << "Using camera." << endl;
    }
    else
    {
        cap.open(video_path);
        cout << "video path:\n" << video_path << endl;
    }
    if (!cap.isOpened())
        return -1;

    // get and set camera parameters
    if (video_path == "0")
    {
        cap.set(CV_CAP_PROP_FRAME_WIDTH, document["camera"]["frame_width"].GetUint());      // 宽度
        cap.set(CV_CAP_PROP_FRAME_HEIGHT, document["camera"]["frame_height"].GetUint());    // 高度
        cap.set(CV_CAP_PROP_FPS, document["camera"]["frame_fps"].GetDouble());              // 帧率
        cap.set(CV_CAP_PROP_BRIGHTNESS, document["camera"]["brightness"].GetDouble());      // 亮度
        cap.set(CV_CAP_PROP_CONTRAST, document["camera"]["contrast"].GetDouble());          // 对比度
        cap.set(CV_CAP_PROP_SATURATION, document["camera"]["saturation"].GetDouble());      // 饱和度
        cap.set(CV_CAP_PROP_HUE, document["camera"]["hue"].GetDouble());                    // 色调
        cap.set(CV_CAP_PROP_EXPOSURE, document["camera"]["exposure"].GetDouble());          // 曝光

        cout << "FRAME_WIDTH: " << cap.get(CV_CAP_PROP_FRAME_WIDTH) << endl;
        cout << "FRAME_HEIGHT: " << cap.get(CV_CAP_PROP_FRAME_HEIGHT) << endl;
        cout << "FPS: " << cap.get(CV_CAP_PROP_FPS) << endl;
        cout << "BRIGHTNESS: " << cap.get(CV_CAP_PROP_BRIGHTNESS) << endl;
        cout << "CONTRAST: " << cap.get(CV_CAP_PROP_CONTRAST) << endl;
        cout << "SATURATION: " << cap.get(CV_CAP_PROP_SATURATION) << endl;
        cout << "HUE: " << cap.get(CV_CAP_PROP_HUE) << endl;
        cout << "EXPOSURE: " << cap.get(CV_CAP_PROP_EXPOSURE) << endl;
    }

    // get video parameters
    double frame_fps = document["video"]["frame_fps"].GetDouble();
    double frame_time = 1000.0 / frame_fps;
    cout << "frame time: " << frame_time << endl;

    // creat table before processing
    createGammaTable(document["image_processing"]["gamma"].GetDouble());
    cout << "creat gamma table success" << endl;

    // prepare variables and parameters
    Mat frame, hsv, th1, th2, th;
    Mat gamma_img, gamma_hsv, gamma_th, gamma_th1, gamma_th2;

    const rapidjson::Value& rl1 = document["image_processing"]["red_low_1"];
    Scalar red_low_range1 = Scalar(rl1[0].GetDouble(), rl1[1].GetDouble(), rl1[2].GetDouble());
    const rapidjson::Value& rh1 = document["image_processing"]["red_high_1"];
    Scalar red_high_range1 = Scalar(rh1[0].GetDouble(), rh1[1].GetDouble(), rh1[2].GetDouble());
    const rapidjson::Value& rl2 = document["image_processing"]["red_low_2"];
    Scalar red_low_range2 = Scalar(rl2[0].GetDouble(), rl2[1].GetDouble(), rl2[2].GetDouble());
    const rapidjson::Value& rh2 = document["image_processing"]["red_high_2"];
    Scalar red_high_range2 = Scalar(rh2[0].GetDouble(), rh2[1].GetDouble(), rh2[2].GetDouble());

    // const rapidjson::Value& gl = document["image_processing"]["green_low"];
    // Scalar green_low = Scalar(gl[0].GetDouble(), gl[1].GetDouble(), gl[2].GetDouble());
    // const rapidjson::Value& gh = document["image_processing"]["green_high"];
    // Scalar green_high = Scalar(gh[0].GetDouble(), gh[1].GetDouble(), gh[2].GetDouble());

    // const rapidjson::Value& bl = document["image_processing"]["blue_low"];
    // Scalar blue_low = Scalar(bl[0].GetDouble(), bl[1].GetDouble(), bl[2].GetDouble());
    // const rapidjson::Value& bh = document["image_processing"]["blue_high"];
    // Scalar blue_high = Scalar(bh[0].GetDouble(), bh[1].GetDouble(), bh[2].GetDouble());

    const rapidjson::Value& cp1 = document["image_processing"]["circle1_param"];
    vector<double> circle1_params = {cp1["dp"].GetDouble(), cp1["minDist"].GetDouble(), cp1["param1"].GetDouble(), 
    cp1["param2"].GetDouble(), cp1["minRadius"].GetDouble(), cp1["maxRadius"].GetDouble()};
    const rapidjson::Value& cp2 = document["image_processing"]["circle2_param"];
    vector<double> circle2_params = {cp2["dp"].GetDouble(), cp2["minDist"].GetDouble(), cp2["param1"].GetDouble(), 
    cp2["param2"].GetDouble(), cp2["minRadius"].GetDouble(), cp2["maxRadius"].GetDouble()};
    const rapidjson::Value& cp3 = document["image_processing"]["circle3_param"];
    vector<double> circle3_params = {cp3["dp"].GetDouble(), cp3["minDist"].GetDouble(), cp3["param1"].GetDouble(), 
    cp3["param2"].GetDouble(), cp3["minRadius"].GetDouble(), cp3["maxRadius"].GetDouble()};
    
    cout << "paremeters load completed" << endl;

    // creat shared memory
    double buffer[4] = {0};
    CShareMemory csm("obj", 1024);
    u32 length = sizeof(buffer);
    
    // image processing
    while (true)
    {
        double tic = static_cast<double>(getTickCount());
        cap >> frame;
        if (frame.empty())
        {
            printf("Empty!\n");
            return -1;
        }

        // // hsv segmentation
        // cvtColor(frame, hsv, COLOR_BGR2HSV);
        // inRange(hsv, Scalar(150,0,230), Scalar(180,150,256), th1);
        // inRange(hsv, Scalar(0,0,230), Scalar(30,150,256), th2);
        // th = th1 + th2;
        // imshow("th", th);

        // gamma correction
        LUT(frame, lookUpTable, gamma_img);
        cvtColor(gamma_img, gamma_hsv, COLOR_BGR2HSV);
        inRange(gamma_hsv, red_low_range1, red_high_range1, gamma_th1);
        inRange(gamma_hsv, red_low_range2, red_high_range2, gamma_th2);
        gamma_th = gamma_th1 + gamma_th2;
        imshow("gamma_th", gamma_th);

        // circle detect
        vector<Vec3f> circles;
        HoughCircles(gamma_th, circles, CV_HOUGH_GRADIENT, circle1_params[0], circle1_params[1], circle1_params[2], circle1_params[3], circle1_params[4], circle1_params[5]);
        if (!circles.empty())
        {
            Vec3f obj = circles[0];
            printf("%.2f, %.2f, %.2f\n", obj[0], obj[1], obj[2]);
            circle(gamma_img, Point(obj[0], obj[1]), obj[2], Scalar(0,255,0), 4);
            circle(gamma_img, Point(obj[0], obj[1]), 1, Scalar(0,255,0), 4);
            // write shared memory
            buffer[0] = obj[0]; buffer[1] = obj[1];
            buffer[2] = obj[2]; buffer[3] = 1;
            csm.PutToShareMem(buffer, length);
        }
        // If the circle is not found, take the centroid of the color
        else
        {
            Moments m = moments(gamma_th, true);
            if (m.m00 != 0)
            {
                double centroid_x = m.m10 / m.m00;
                double centroid_y = m.m01 / m.m00;
                printf("%.2f, %.2f\n", centroid_x, centroid_y);
                circle(gamma_img, Point(centroid_x, centroid_y), 1, Scalar(0,255,0), 4);
                // write shared memory
                buffer[0] = centroid_x; buffer[1] = centroid_y;
                buffer[2] = 0; buffer[3] = -1;
                csm.PutToShareMem(buffer, length);
            }
            // crossing completed
            else
            {
                buffer[0] = -4; buffer[1] = -4;
                buffer[2] = -4; buffer[3] = -4;
                csm.PutToShareMem(buffer, length);
            }
        }

        imshow("video", gamma_img);

        // keep frame rate
        double toc = static_cast<double>(getTickCount());
        double processing_time = (toc - tic) / getTickFrequency() * 1000;
        printf("Processing Time: %lf ms\n", processing_time);
        if (processing_time < frame_time-1)
        {
            waitKey(int(frame_time - processing_time));
        }
        else
        {
            waitKey(1);
        }
    }

    // finish
    cap.release();
    return 0;
}