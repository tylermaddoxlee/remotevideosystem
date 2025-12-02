#include <opencv2/opencv.hpp>
#include <chrono>
#include <thread>
#include <iostream>

using namespace cv;
using namespace std;

int main(int argc, char** argv)
{
    cout << "Opening camera..." << endl;

    VideoCapture cam(
        "v4l2src device=/dev/video1 do-timestamp=true ! "
        "image/jpeg ! jpegdec ! "
        "videorate ! video/x-raw,framerate=15/1 ! "
        "videoconvert ! "
        "appsink",
        cv::CAP_GSTREAMER
    );

    if (!cam.isOpened()) {
        cerr << "FAILED TO OPEN camera" << endl;
        return -1;
    }

    cout << "Camera opened." << endl;
    cout << "Actual size  : " << cam.get(CAP_PROP_FRAME_WIDTH)
         << " x "          << cam.get(CAP_PROP_FRAME_HEIGHT) << endl;
    cout << "Actual FPS   : " << cam.get(CAP_PROP_FPS) << endl;

    Mat frame, gray, firstFrame, frameDelta, thresh;
    vector<vector<Point>> contours;

    // --- Warm-up: grab a few frames so auto-exposure settles ---
    bool gotWarmup = false;
    for (int i = 0; i < 40; ++i) {
        cam >> frame;
        if (!frame.empty()) {
            gotWarmup = true;
            break;
        }
        cerr << "Warm-up: empty frame, retrying...\n";
        this_thread::sleep_for(chrono::milliseconds(50));
    }

    if (!gotWarmup) {
        cerr << "Camera opened but no frames during warm-up\n";
        return -1;
    }

    // --- Capture initial background frame ---
    cam >> frame;
    if (frame.empty()) {
        cerr << "Failed to grab initial frame\n";
        return -1;
    }
    cvtColor(frame, firstFrame, COLOR_BGR2GRAY);
    GaussianBlur(firstFrame, firstFrame, Size(21, 21), 0);

    const int MIN_MOTION_AREA = 800;

    cout << "Starting motion detection (headless). Press Ctrl+C to quit.\n";

    while (true) {
        if (!cam.read(frame) || frame.empty()) {
            cerr << "Empty frame from camera, exiting.\n";
            break;
        }

        cvtColor(frame, gray, COLOR_BGR2GRAY);
        GaussianBlur(gray, gray, Size(21, 21), 0);

        absdiff(firstFrame, gray, frameDelta);
        threshold(frameDelta, thresh, 25, 255, THRESH_BINARY);
        dilate(thresh, thresh, Mat(), Point(-1,-1), 2);

        findContours(thresh, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        bool motionDetected = false;
        for (const auto& c : contours) {
            if (contourArea(c) < MIN_MOTION_AREA)
                continue;
            motionDetected = true;
        }

        if (motionDetected) {
            cout << "[MOTION] Motion detected at "
                 << chrono::duration_cast<chrono::milliseconds>(
                        chrono::steady_clock::now().time_since_epoch()
                    ).count()
                 << " ms\n";
        }

        // small sleep to avoid spamming the CPU
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    cam.release();
    return 0;
}

