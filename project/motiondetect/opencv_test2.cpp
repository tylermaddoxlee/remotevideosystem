#include <opencv2/opencv.hpp>
#include <chrono>
#include <thread>
#include <iostream>
#include <signal.h>

using namespace cv;
using namespace std;

static bool running = true;
void handleSignal(int) { running = false; }

int main(int argc, char** argv)
{
    signal(SIGINT, handleSignal);

    string device = "/dev/video3";
    if (argc > 1) device = argv[1];

    VideoCapture cam(device, cv::CAP_V4L2);
    if (!cam.isOpened()) {
        cerr << "FAILED TO OPEN camera: " << device << endl;
        return -1;
    }

    cam.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cam.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    cam.set(cv::CAP_PROP_FPS, 15);

    Mat frame, gray, firstFrame, frameDelta, thresh;
    vector<vector<Point>> contours;

    // Warm-up
    for (int i = 0; i < 5; i++) {
        cam >> frame;
        this_thread::sleep_for(chrono::milliseconds(30));
    }

    // Background frame
    cam >> frame;
    if (frame.empty()) return -1;

    cvtColor(frame, firstFrame, COLOR_BGR2GRAY);
    GaussianBlur(firstFrame, firstFrame, Size(21, 21), 0);

    const int MIN_MOTION_AREA = 2500;

    while (running) {

        if (!cam.read(frame) || frame.empty()) break;

        cvtColor(frame, gray, COLOR_BGR2GRAY);
        GaussianBlur(gray, gray, Size(21, 21), 0);

        absdiff(firstFrame, gray, frameDelta);
        threshold(frameDelta, thresh, 25, 255, THRESH_BINARY);
        dilate(thresh, thresh, Mat(), Point(-1,-1), 2);

        findContours(thresh, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        bool motionDetected = false;
        for (auto &c : contours) {
            if (contourArea(c) < MIN_MOTION_AREA)
                continue;

            motionDetected = true;
            Rect box = boundingRect(c);
            rectangle(frame, box, Scalar(0, 0, 255), 2);  // overlay box
        }

        // Encode as JPEG
        vector<uchar> buf;
        imencode(".jpg", frame, buf);

        // Write JPEG to stdout for GStreamer
        cout.write((char*)buf.data(), buf.size());
        cout.flush();
    }

    cam.release();
    return 0;
}
