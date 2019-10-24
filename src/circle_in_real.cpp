#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include "rapidjson/document.h"
using namespace cv;
using namespace std;

#define expand_thre 3 * CV_PI / 180
#define rho_thre 50
#define INF 0x3f3f3f3f

//! [changing-contrast-brightness-gamma-correction]
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
    createGammaTable(2);

    // image processing
    Mat frame, hsv, th1, th2, th;
    while (true)
    {
        double tic = static_cast<double>(getTickCount());
        cap >> frame;
        if (frame.empty())
        {
            printf("Empty!\n");
            return -1;
        }
        cvtColor(frame, hsv, COLOR_BGR2HSV);

        // hsv segmentation
        inRange(hsv, Scalar(150,0,230), Scalar(180,150,256), th1);
        inRange(hsv, Scalar(0,0,230), Scalar(30,150,256), th2);
        th = th1 + th2;
        imshow("th", th);

        // gamma correction
        Mat gamma_img, gamma_hsv, gamma_th, gamma_th1, gamma_th2;
        LUT(frame, lookUpTable, gamma_img);
        imshow("gamma", gamma_img);
        cvtColor(gamma_img, gamma_hsv, COLOR_BGR2HSV);
        inRange(gamma_hsv, Scalar(170,150,230), Scalar(180,255,256), gamma_th1);
        inRange(gamma_hsv, Scalar(0,150,230), Scalar(20,255,256), gamma_th2);
        gamma_th = gamma_th1 + gamma_th2;
        imshow("gamma_th", gamma_th);


        vector<Vec3f> circles;
        HoughCircles(th, circles, CV_HOUGH_GRADIENT, 4, 100, 100, 160, 0, 0);
        if (!circles.empty())
        {
            Vec3f obj = circles[0];
            printf("%.2f, %.2f, %.2f\n", obj[0], obj[1], obj[2]);
            circle(frame, Point(obj[0], obj[1]), obj[2], Scalar(0,255,0), 4);
        }

        // std::vector<Vec4i> lines;
        // cv::HoughLinesP(th, lines, 2, 2*CV_PI/180, 300, 0, 400);
        // printf("lines: %zu\n", lines.size());
        // for (size_t i=0; i<lines.size(); i++)
        // {
        //     line(frame, Point(lines[i][0], lines[i][1]), Point(lines[i][2], lines[i][3]), Scalar(255, 0, 0), 4);
        // }

        vector<Vec2f> lines;
        Mat hough_img(800, 180, CV_8UC1, Scalar(0));
        HoughLines(th, lines, 2, CV_PI/90, 200, 0, 0);

        // vector<Square> squares;
        // findSquare(lines, squares);
        // for (size_t i = 0; i < squares.size(); i++)
        // {
        //     drawSquare(frame, squares[i]);
        // }
        // printf("Squares: %zu\n", squares.size());

        for (size_t i = 0; i < lines.size(); i++)
        {
            float rho = lines[i][0], theta = lines[i][1];
            Point pt1, pt2;
            double a = cos(theta), b = sin(theta);
            double x0 = a * rho, y0 = b * rho;
            pt1.x = cvRound(x0 + 500 * (-b));
            pt1.y = cvRound(y0 + 500 * (a));
            pt2.x = cvRound(x0 - 500 * (-b));
            pt2.y = cvRound(y0 - 500 * (a));
            // line(frame, pt1, pt2, Scalar(255, 0, 0), 8);
            circle(hough_img, Point(theta * 180 / CV_PI, rho), 1, Scalar(255), 4);
        }

        cv::imshow("video", frame);
        cv::imshow("hough_img", hough_img);

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