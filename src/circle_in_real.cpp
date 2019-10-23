#include <iostream>
#include <opencv2/opencv.hpp>
#include <python2.7/Python.h>
using namespace cv;

#define expand_thre 3 * CV_PI / 180
#define rho_thre 50
#define INF 0x3f3f3f3f

struct Square
{
    std::vector<Vec2f> edges;
};

bool cmp(Vec2f &a, Vec2f &b)
{
    return a[1] < b[1];
}

void findMaxMin(std::vector<Vec2f> &lines, Vec2f &max, Vec2f &min)
{
    max[0] = -INF, min[0] = INF;
    for (size_t i = 0; i < lines.size()-1; i++)
    {
        if (lines[i][0] < lines[i+1][0])
        {
            if (lines[i+1][0] > max[0])
                max = lines[i+1];
            if (lines[i][0] < min[0])
                min = lines[i];
        }
        else
        {
            if (lines[i][0] > max[0])
                max = lines[i];
            if (lines[i+1][0] < min[0])
                min = lines[i+1];
        }
    }
}

inline float Mod(float a)
{
    if (a > CV_PI)
        return a - CV_PI;
    else if (a < 0)
        return a + CV_PI;
    else
        return a;
}

void drawSquare(Mat &img, Square &s)
{
    for (size_t i = 0; i < 4; i++)
    {
        float rho = s.edges[i][0], theta = s.edges[i][1];
        Point pt1, pt2;
        double a = cos(theta), b = sin(theta);
        double x0 = a * rho, y0 = b * rho;
        pt1.x = cvRound(x0 + 500 * (-b));
        pt1.y = cvRound(y0 + 500 * (a));
        pt2.x = cvRound(x0 - 500 * (-b));
        pt2.y = cvRound(y0 - 500 * (a));
        line(img, pt1, pt2, Scalar(255, 0, 0), 8);
    }
}

bool expand(std::vector<Vec2f> &lines, size_t index, float value, std::vector<Vec2f> &out)
{
    if (lines[index][1] < Mod(value - expand_thre) || lines[index][1] > Mod(value + expand_thre))
        return false;
    else
    {
        size_t i = index;
        while (lines[i][1] >= Mod(value - expand_thre) && i > 0)
        {
            out.push_back(lines[i]);
            i--;
        }
        while (lines[i][1] <= Mod(value + expand_thre) && i < lines.size())
        {
            out.push_back(lines[i]);
            i++;
        }
    }
    return true;
}

bool findVer(std::vector<Vec2f> &lines, size_t i, std::vector<Vec2f> &ver)
{
    int index = 0;
    int low = 0;
    int high = lines.size() - 1;
    int value = Mod(lines[i][1] + CV_PI/2);
    while (low <= high)
    {
        int mid = low + (high - low) / 2;
        index = mid;
        if (lines[mid][1] > value)
            high = mid -1;
        else
            low = mid + 1;
    }
    return expand(lines, index, value, ver);
}

void isSquare(std::vector<Vec2f> &ori, std::vector<Vec2f> &ver, std::vector<Square> &squares)
{
    Vec2f max_ori = ori[0], min_ori = ori[0];
    Vec2f max_ver = ver[0], min_ver = ver[0];
    findMaxMin(ori, max_ori, min_ori);
    findMaxMin(ver, max_ver, min_ver);
    if (max_ori[0] - min_ori[0] > rho_thre && max_ver[0] - min_ver[0] > rho_thre)
    {
        Square s;
        s.edges.push_back(max_ori);
        s.edges.push_back(max_ver);
        s.edges.push_back(min_ori);
        s.edges.push_back(min_ver);
        squares.push_back(s);
    }
}

void findSquare(std::vector<Vec2f> &lines, std::vector<Square> &squares)
{
    std::sort(lines.begin(), lines.end(), cmp);
    for (size_t i = 0; i < lines.size(); i++)
    {
        std::vector<Vec2f> ver_lines, ori_lines;
        if (findVer(lines, i, ver_lines))
        {
            if (expand(lines, i, lines[i][1], ori_lines))
            {
                isSquare(ori_lines, ver_lines, squares);
            }
            else
                continue;
        }
        else
            continue;
    }
}

//! [changing-contrast-brightness-gamma-correction]
Mat lookUpTable(1, 256, CV_8U);
void createGammaTable(const double gamma_)
{
    uchar* p = lookUpTable.ptr();
    for( int i = 0; i < 256; ++i)
        p[i] = saturate_cast<uchar>(pow(i / 255.0, gamma_) * 255.0);
}

void setISO(int iso)
{
    Py_Initialize();
    if ( !Py_IsInitialized() ) {  
        printf("Python script initialize failed!\n");  
    }
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('../src/')"); //放在cpp的同一路径下

    PyObject *pName,*pModule,*pDict,*pFunc,*pArgs;
    pName = PyString_FromString("iso");
    pModule = PyImport_Import(pName);
    if ( !pModule ) {
        printf("Can't find iso.py");
        getchar();
    }
    pDict = PyModule_GetDict(pModule);
    pFunc = PyDict_GetItemString(pDict, "ISO");
    if ( !pFunc || !PyCallable_Check(pFunc) ) {  
        printf("can't find function [ISO]");
    }
    *pArgs;
    pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, Py_BuildValue("i", iso));
    PyObject_CallObject(pFunc, pArgs);
    Py_Finalize();
}


int main(int argc, char **argv)
{
    setISO(50);

    if (argc != 2)
    {
        printf("Usage: %s VIDEO_PATH\n", argv[0]);
    }

    VideoCapture cap;
    if (*(argv[1]) == '0')
        cap.open(0);
    else
        cap.open(argv[1]);
        printf("%s\n", argv[1]);
    if (!cap.isOpened())
        return -1;

    double frame_time = 1000.0/27;
    Mat frame, hsv, th1, th2, th;
    createGammaTable(2);

    while (true)
    {
        double tic = static_cast<double>(getTickCount());
        cap >> frame;
        if (frame.empty())
        {
            printf("Empty!\n");
            break;
        }
        resize(frame, frame, Size(640, 480));
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


        std::vector<Vec3f> circles;
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

        std::vector<Vec2f> lines;
        Mat hough_img(800, 180, CV_8UC1, Scalar(0));
        HoughLines(th, lines, 2, CV_PI/90, 200, 0, 0);

        std::vector<Square> squares;
        findSquare(lines, squares);
        for (size_t i = 0; i < squares.size(); i++)
        {
            drawSquare(frame, squares[i]);
        }
        printf("Squares: %zu\n", squares.size());

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
    cap.release();
    return 0;
}