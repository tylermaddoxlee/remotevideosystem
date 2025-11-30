#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <csignal>
#include <unistd.h> // for write()

using namespace cv;
using namespace std;

static bool running = true;
void handleSignal(int) { running = false; }

int main(int argc, char** argv)
{
    signal(SIGINT, handleSignal);

    // Select camera device (default: /dev/video3)
    string device = "/dev/video3";
    if (argc > 1) {
        device = argv[1];
    }

    VideoCapture cam(device, cv::CAP_V4L2);
    if (!cam.isOpened()) {
        cerr << "ERROR: Failed to open camera: " << device << endl;
        return -1;
    }

    // Camera settings
    cam.set(CAP_PROP_FRAME_WIDTH, 640);
    cam.set(CAP_PROP_FRAME_HEIGHT, 480);
    cam.set(CAP_PROP_FPS, 15);

    Mat frame, gray, firstFrame, frameDelta, thresh;
    vector<vector<Point>> contours;

    // Warm-up frames
    bool gotWarmup = false;
        for (int i = 0; i < 40; ++i) {  // try for ~2 seconds
            cam >> frame;
            if (!frame.empty()) {
                gotWarmup = true;
                break;
            }
            cerr << "Warm-up: empty frame, retrying...\n";
            this_thread::sleep_for(chrono::milliseconds(50));
        }

        if (!gotWarmup) {
            cerr << "Camera opened but no frames during warm-up (VM / USB issue)\n";
            return -1;
        }

    // Capture initial background frame
    cam >> frame;
    if (frame.empty()) {
        cerr << "ERROR: Failed to grab initial frame\n";
        return -1;
    }
    cvtColor(frame, firstFrame, COLOR_BGR2GRAY);
    GaussianBlur(firstFrame, firstFrame, Size(21, 21), 0);

    const int MIN_MOTION_AREA = 2500;

    while (running) {

        if (!cam.read(frame) || frame.empty()) {
            cerr << "ERROR: Camera disconnected or empty frame\n";
            break;
        }

        // Convert to grayscale and blur
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        GaussianBlur(gray, gray, Size(21, 21), 0);

        // Motion detection
        absdiff(firstFrame, gray, frameDelta);
        threshold(frameDelta, thresh, 25, 255, THRESH_BINARY);
        dilate(thresh, thresh, Mat(), Point(-1,-1), 2);
        findContours(thresh, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        for (auto &c : contours) {
            if (contourArea(c) < MIN_MOTION_AREA)
                continue;
            Rect box = boundingRect(c);
            rectangle(frame, box, Scalar(0, 0, 255), 2);
        }

        // Encode frame as JPEG
        vector<uchar> buf;
        if (!imencode(".jpg", frame, buf)) {
            cerr << "ERROR: Failed to encode JPEG\n";
            continue;
        }

        // Output JPEG bytes to stdout (for GStreamer fdsrc)
        ssize_t written = write(STDOUT_FILENO, buf.data(), buf.size());
        if (written < 0) {
            cerr << "ERROR: write() to stdout failed\n";
            break;
        }

        fsync(STDOUT_FILENO); // ensure output flush
    }

    cam.release();
    return 0;
}
