/*
 * Simple motion detection with OpenCV (Linux + V4L2)
 *
 * Compile:
 *   g++ opencv_test2.cpp -o motioncam `pkg-config --cflags --libs opencv4`
 *
 * Run:
 *   ./motioncam /dev/video3
 */

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
    // -------------------------------
    // Select camera device
    // -------------------------------
    string device = "/dev/video3";   // Default for your webcam MAY NEED TO CHANGE TO YOURS

    if (argc > 1) {
        device = argv[1];
    }

    cout << "Opening camera: " << device << endl;

    // Open camera using V4L2 backend
    VideoCapture cam(device, cv::CAP_V4L2);

    if (!cam.isOpened()) {
        cerr << "FAILED TO OPEN camera: " << device << endl;
        return -1;
    }
    cout << "Camera opened successfully from " << device << endl;

    // -------------------------------
    // Set camera properties
    // -------------------------------
    cam.set(cv::CAP_PROP_FOURCC, VideoWriter::fourcc('Y','U','Y','V'));
    cam.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cam.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    cam.set(cv::CAP_PROP_FPS, 15);

    // Display actual camera settings
    int fi = (int)cam.get(cv::CAP_PROP_FOURCC);
    char fourcc[5] = {
        (char)(fi & 0xFF), (char)((fi >> 8) & 0xFF),
        (char)((fi >> 16) & 0xFF), (char)((fi >> 24) & 0xFF),
        0
    };
    cout << "Actual FOURCC: " << fourcc << endl;
    cout << "Actual size: " << cam.get(CAP_PROP_FRAME_WIDTH)
         << "x" << cam.get(CAP_PROP_FRAME_HEIGHT) << endl;
    cout << "Actual FPS: " << cam.get(CAP_PROP_FPS) << endl;

    // -------------------------------
    // Warm-up
    // -------------------------------
    Mat frame, gray, firstFrame, frameDelta, thresh;
    vector<vector<Point>> contours;

    for (int i = 0; i < 10; ++i) {
        cam >> frame;
        if (frame.empty()) {
            cerr << "Failed to grab warm-up frame\n";
            return -1;
        }
        this_thread::sleep_for(chrono::milliseconds(50));
    }

    // -------------------------------
    // Take background frame
    // -------------------------------
    cam >> frame;
    if (frame.empty()) {
        cerr << "Failed to grab initial frame\n";
        return -1;
    }
    cvtColor(frame, firstFrame, COLOR_BGR2GRAY);
    GaussianBlur(firstFrame, firstFrame, Size(21, 21), 0);

    const int MIN_MOTION_AREA = 2500;

    cout << "Starting motion detection. Press ESC or q to quit.\n";

    // -------------------------------
    // Motion detection loop
    // -------------------------------
    while (running) {

        if (!cam.read(frame) || frame.empty()) {
            cerr << "Camera disconnected or empty frame.\n";
            break;
        }

        cvtColor(frame, gray, COLOR_BGR2GRAY);
        GaussianBlur(gray, gray, Size(21, 21), 0);

        // Frame difference
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
            rectangle(frame, box, Scalar(0, 0, 255), 2);
        }

        if (motionDetected) {
            putText(frame, "Motion Detected", Point(10, 30),
                    FONT_HERSHEY_SIMPLEX, 0.9, Scalar(0,0,255), 2);
        }

        char key = (char)waitKey(1);

        if (key == 27 || key == 'q') {
            break;
        } else if (key == 'r') {
            cout << "Background reset.\n";
            cvtColor(frame, firstFrame, COLOR_BGR2GRAY);
            GaussianBlur(firstFrame, firstFrame, Size(21, 21), 0);
        }
    }

    cam.release();
    destroyAllWindows();
    return 0;
}
