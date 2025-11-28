/*
 * Simple motion detection with OpenCV (Linux + V4L2)
 *
 * Compile on Debian:
 *   g++ opencv_test2.cpp -o opencv_test2 `pkg-config --cflags --libs opencv4`
 *
 * Run (NO sudo):
 *   ./opencv_test2
 * 
 * DO THESE BEFORE RUNNING ON VM!!:
 * sudo apt install libopencv-dev
 * switch to 3.1 USB, connect in removable devices
 */

#include <opencv2/opencv.hpp>
#include <chrono>
#include <thread>
#include <iostream>

using namespace cv;
using namespace std;

int main(int argc, char** argv)
{
    cout << "Opening camera..." << endl;

    // /dev/video0, V4L2 backend (same call you’ll use on BeagleY)
    VideoCapture cam(0, cv::CAP_V4L2);
    if (!cam.isOpened()) {
        cerr << "FAILED TO OPEN camera" << endl;
        return -1;
    }
    cout << "Camera opened." << endl;

    // Force a format that is stable in VMware: YUYV, 640x480, 15 fps
    cam.set(cv::CAP_PROP_FOURCC, VideoWriter::fourcc('Y','U','Y','V'));
    cam.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cam.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    cam.set(cv::CAP_PROP_FPS, 15);

    // Print what we actually got (useful on BeagleY too)
    int fi = static_cast<int>(cam.get(cv::CAP_PROP_FOURCC));
    char fourcc[5] = {
        (char)(fi & 0xFF),
        (char)((fi >> 8) & 0xFF),
        (char)((fi >> 16) & 0xFF),
        (char)((fi >> 24) & 0xFF),
        0
    };
    cout << "Actual FOURCC: " << fourcc << endl;
    cout << "Actual size : " << cam.get(CAP_PROP_FRAME_WIDTH)
         << " x "          << cam.get(CAP_PROP_FRAME_HEIGHT) << endl;
    cout << "Actual FPS  : " << cam.get(CAP_PROP_FPS) << endl;

    Mat frame, gray, firstFrame, frameDelta, thresh;
    vector<vector<Point>> contours;

    // --- Warm-up: grab a few frames so auto-exposure settles ---
    for (int i = 0; i < 10; ++i) {
        cam >> frame;
        if (frame.empty()) {
            cerr << "Failed to grab warm-up frame\n";
            return -1;
        }
        this_thread::sleep_for(chrono::milliseconds(50));
    }

    // --- Capture initial background frame ---
    cam >> frame;
    if (frame.empty()) {
        cerr << "Failed to grab initial frame\n";
        return -1;
    }
    cvtColor(frame, firstFrame, COLOR_BGR2GRAY);
    GaussianBlur(firstFrame, firstFrame, Size(21, 21), 0);

    const int MIN_MOTION_AREA = 800;  // tweak for sensitivity

    cout << "Starting motion detection. Press ESC or q to quit.\n";

    while (true) {
        if (!cam.read(frame) || frame.empty()) {
            cerr << "Empty frame from camera, exiting.\n";
            break;
        }

        // Convert current frame to grayscale + blur (noise reduction)
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        GaussianBlur(gray, gray, Size(21, 21), 0);

        // Frame difference vs background
        absdiff(firstFrame, gray, frameDelta);
        threshold(frameDelta, thresh, 25, 255, THRESH_BINARY);
        dilate(thresh, thresh, Mat(), Point(-1,-1), 2);

        // Find motion blobs
        findContours(thresh, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        bool motionDetected = false;
        for (const auto& c : contours) {
            if (contourArea(c) < MIN_MOTION_AREA)
                continue;

            motionDetected = true;
            Rect box = boundingRect(c);
            rectangle(frame, box, Scalar(0, 0, 255), 2);
        }

        if (motionDetected) {
            putText(frame, "Motion Detected",
                    Point(10, 25),
                    FONT_HERSHEY_SIMPLEX, 0.8,
                    Scalar(0, 0, 255), 2);
        }

        // Show debug windows (you can remove thresh/frameDelta later)
        imshow("Camera", frame);
        // imshow("Thresh", thresh);
        // imshow("Delta", frameDelta);

        // Key handling: ESC or 'q' to quit, 'r' to reset background
        int key = waitKey(1) & 0xFF;
        if (key == 27 || key == 'q') {
            break;
        } else if (key == 'r') {
            cout << "Resetting background frame.\n";
            cvtColor(frame, firstFrame, COLOR_BGR2GRAY);
            GaussianBlur(firstFrame, firstFrame, Size(21, 21), 0);
        }
    }

    cam.release();
    destroyAllWindows();
    return 0;
}
