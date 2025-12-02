#include <opencv2/opencv.hpp>
#include <chrono>
#include <thread>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>

using namespace cv;
using namespace std;

// ---- CONFIG ----
// IP/port of your Debian VM / Node server (where server.js is running)
static const char* MOTION_SERVER_IP   = "192.168.7.1";
static const int   MOTION_SERVER_PORT = 12346;   // motion UDP port

// Beagle local TCP MJPEG stream (from gst-launch tcpserversink)
static const char* MJPEG_HOST = "127.0.0.1";     // Beagle itself
static const int   MJPEG_PORT = 8554;           // must match tcpserversink port

// Where to save clips (must be visible to both Beagle & Debian VM via NFS)
static const std::string CLIPS_DIR = "/mnt/remote/project/beagley-server/clips";

bool ensureDirExists(const std::string& path) {
    struct stat st {};
    if (stat(path.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    if (mkdir(path.c_str(), 0755) == 0) {
        std::cout << "[REC] Created clips directory: " << path << std::endl;
        return true;
    }
    perror("mkdir clips");
    return false;
}

int main(int argc, char** argv)
{
    cout << "Connecting to local MJPEG stream for motion detection..." << endl;

    if (!ensureDirExists(CLIPS_DIR)) {
        cerr << "Failed to ensure clips directory exists: " << CLIPS_DIR << endl;
        return -1;
    }

    // GStreamer pipeline (reads the same MJPEG you're streaming out)
    std::string pipeline =
        "tcpclientsrc host=" + std::string(MJPEG_HOST) +
        " port=" + std::to_string(MJPEG_PORT) + " ! "
        "multipartdemux ! "
        "jpegdec ! "
        "videoconvert ! video/x-raw,format=BGR ! "
        "appsink";

    VideoCapture cam(pipeline, cv::CAP_GSTREAMER);

    if (!cam.isOpened()) {
        cerr << "FAILED TO OPEN MJPEG pipeline" << endl;
        return -1;
    }

    cout << "Connected to MJPEG stream." << endl;
    cout << "Reported size  : " << cam.get(CAP_PROP_FRAME_WIDTH)
         << " x "            << cam.get(CAP_PROP_FRAME_HEIGHT) << endl;
    cout << "Reported FPS   : " << cam.get(CAP_PROP_FPS) << endl;

    // ----- Setup UDP socket for motion events -----
    int motionSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (motionSock < 0) {
        perror("socket");
        return -1;
    }

    sockaddr_in motionAddr {};
    motionAddr.sin_family = AF_INET;
    motionAddr.sin_port   = htons(MOTION_SERVER_PORT);
    if (inet_pton(AF_INET, MOTION_SERVER_IP, &motionAddr.sin_addr) != 1) {
        cerr << "Invalid MOTION_SERVER_IP\n";
        close(motionSock);
        return -1;
    }

    auto sendMotionEvent = [&](const char* msg) {
        sendto(motionSock, msg, strlen(msg), 0,
               (sockaddr*)&motionAddr, sizeof(motionAddr));
    };

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
        cerr << "Warm-up: empty frame from stream, retrying...\n";
        this_thread::sleep_for(chrono::milliseconds(50));
    }

    if (!gotWarmup) {
        cerr << "Connected to stream but no frames during warm-up\n";
        close(motionSock);
        return -1;
    }

    // --- Capture initial background frame ---
    cam >> frame;
    if (frame.empty()) {
        cerr << "Failed to grab initial frame from stream\n";
        close(motionSock);
        return -1;
    }

    cvtColor(frame, firstFrame, COLOR_BGR2GRAY);
    GaussianBlur(firstFrame, firstFrame, Size(21, 21), 0);

    const int framePixels = frame.cols * frame.rows;

    // === TUNING KNOBS (easy to adjust) ===
    const double MIN_AREA_RATIO = 0.002;   // 0.2% of frame
    const int MIN_MOTION_AREA   = static_cast<int>(framePixels * MIN_AREA_RATIO);

    const double HIGH_RATIO = 0.01;   // 1% of frame => definitely motion
    const double LOW_RATIO  = 0.003;  // 0.3% of frame => quiet enough

    const double HIGH_AREA = framePixels * HIGH_RATIO;
    const double LOW_AREA  = framePixels * LOW_RATIO;

    const int IDLE_FRAMES_REQUIRED = 10;  // ~0.7s at 15fps

    cout << "Frame pixels: " << framePixels
         << " | MIN_MOTION_AREA ~ " << MIN_MOTION_AREA
         << " | HIGH_AREA ~ "      << HIGH_AREA
         << " | LOW_AREA ~ "       << LOW_AREA << endl;

    // Motion state
    enum MotionState { IDLE, MOTION };
    MotionState state = IDLE;
    int framesBelowLow = 0;

    auto lastMotionSend  = chrono::steady_clock::now();
    auto motionStartTime = chrono::steady_clock::now();
    const auto MOTION_COOLDOWN       = chrono::milliseconds(300); // ≥ ~3/sec
    const auto MAX_CONTINUOUS_MOTION = chrono::seconds(8);        // after this, assume new background

    // --- Recording state ---
    VideoWriter writer;
    bool recording = false;
    std::string currentClipName;

    auto startRecording = [&](const Mat& firstFrameForClip) {
        if (recording) return;

        std::time_t t = std::time(nullptr);
        std::tm tm{};
        localtime_r(&t, &tm);

        char buf[64];
        std::strftime(buf, sizeof(buf), "clip_%Y%m%d_%H%M%S.avi", &tm);
        currentClipName = std::string(buf);

        std::string fullPath = CLIPS_DIR + "/" + currentClipName;

        int fourcc = VideoWriter::fourcc('M', 'J', 'P', 'G');
        double fps = 15.0;
        Size size(firstFrameForClip.cols, firstFrameForClip.rows);

        if (!writer.open(fullPath, fourcc, fps, size, true)) {
            std::cerr << "[REC] Failed to open writer for " << fullPath << std::endl;
            recording = false;
            return;
        }

        recording = true;
        std::cout << "[REC] Started clip: " << currentClipName << std::endl;

        // Write the first frame immediately
        writer.write(firstFrameForClip);
    };

    auto stopRecording = [&]() {
        if (!recording) return;
        writer.release();
        recording = false;
        std::cout << "[REC] Stopped clip: " << currentClipName << std::endl;
    };

    cout << "Starting motion detection with hysteresis + recording. Ctrl+C to quit.\n";

    while (true) {
        if (!cam.read(frame) || frame.empty()) {
            cerr << "Empty frame from stream, exiting.\n";
            break;
        }

        // Always record the current frame if we are in a clip
        if (recording) {
            writer.write(frame);
        }

        // Convert to grayscale + blur
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        GaussianBlur(gray, gray, Size(21, 21), 0);

        // Difference vs background
        absdiff(firstFrame, gray, frameDelta);

        // Threshold: 25 is a bit more sensitive than 30
        threshold(frameDelta, thresh, 25, 255, THRESH_BINARY);

        // Clean up noise
        erode(thresh, thresh, Mat(), Point(-1, -1), 1);
        dilate(thresh, thresh, Mat(), Point(-1, -1), 2);

        findContours(thresh, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        double totalMotionArea = 0.0;
        for (const auto& c : contours) {
            double area = contourArea(c);
            if (area < MIN_MOTION_AREA)
                continue;
            totalMotionArea += area;
        }

        auto now = chrono::steady_clock::now();

        if (state == IDLE) {
            // --- IDLE ---
            if (totalMotionArea > HIGH_AREA) {
                // Enter motion state
                state = MOTION;
                motionStartTime = now;
                framesBelowLow = 0;

                // Start recording clip
                startRecording(frame);

                if (now - lastMotionSend > MOTION_COOLDOWN) {
                    cout << "[MOTION] START (area = " << totalMotionArea << ")\n";
                    sendMotionEvent("MOTION\n");
                    lastMotionSend = now;
                }

                // While moving, adapt background slowly
                addWeighted(gray, 0.02, firstFrame, 0.98, 0.0, firstFrame);

            } else {
                // Still idle: adapt background moderately
                addWeighted(gray, 0.08, firstFrame, 0.92, 0.0, firstFrame);
            }

        } else {
            // --- MOTION ---
            // Long continuous motion => treat current frame as new background
            if (now - motionStartTime > MAX_CONTINUOUS_MOTION) {
                cout << "[SCENE] Long motion, treating current frame as new background, back to IDLE\n";
                gray.copyTo(firstFrame);
                state = IDLE;
                framesBelowLow = 0;
                stopRecording();
                this_thread::sleep_for(chrono::milliseconds(5));
                continue;
            }

            if (totalMotionArea > HIGH_AREA) {
                // sustained strong motion
                if (now - lastMotionSend > MOTION_COOLDOWN) {
                    cout << "[MOTION] Motion (area = " << totalMotionArea << ")\n";
                    sendMotionEvent("MOTION\n");
                    lastMotionSend = now;
                }
                framesBelowLow = 0;
                addWeighted(gray, 0.01, firstFrame, 0.99, 0.0, firstFrame);

            } else if (totalMotionArea < LOW_AREA) {
                // quiet: count frames towards idle
                framesBelowLow++;
                addWeighted(gray, 0.20, firstFrame, 0.80, 0.0, firstFrame);

                if (framesBelowLow >= IDLE_FRAMES_REQUIRED) {
                    cout << "[IDLE] Back to idle (area = " << totalMotionArea << ")\n";
                    state = IDLE;
                    framesBelowLow = 0;
                    stopRecording();
                }

            } else {
                // small wobbles
                framesBelowLow = 0;
                addWeighted(gray, 0.06, firstFrame, 0.94, 0.0, firstFrame);
            }
        }

        this_thread::sleep_for(chrono::milliseconds(5));
    }

    stopRecording();
    close(motionSock);
    cam.release();
    return 0;
}
