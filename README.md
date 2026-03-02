# remotevideosystem
## Overview:
This project implements a real-time embedded vision system capable of live video streaming, motion detection, and remote hardware control. The system runs on Linux-based embedded platforms and integrates OpenCV-based image processing with UDP streaming and WebSocket relaying to a browser client. Hardware peripherals including joystick input (SPI ADC) and servo motor control (PWM) are interfaced directly from C/C++ firmware.

![image alt](https://github.com/tylermaddoxlee/remotevideosystem/blob/2aff2d9060c08fdea00ce91213d5d80cd99ec62e/project/ENSC351%20-%20Demo.webp)

## System Architecture:

Camera Capture → OpenCV Processing Pipeline → UDP Frame Transmission → WebSocket Relay Server → Browser Client Rendering

## Software running on localhost 

![image alt](https://github.com/tylermaddoxlee/remotevideosystem/blob/2aff2d9060c08fdea00ce91213d5d80cd99ec62e/project/unnamed.png)

## Technical Highlights

- Multithreaded video capture and transmission
- Real-time motion detection using frame differencing
- Low-latency UDP frame streaming
- SPI-based joystick input via MCP3208 ADC
- PWM servo motor control from C++ firmware

## My Contributions

- Implemented UDP streaming modules in C/C++
- Integrated OpenCV motion detection pipeline
- WebSocket relay integration
- Developed SPI ADC interface for joystick input

## Future Improvements
- Integrate object detection model (e.g., YOLO)
- Add adaptive thresholding for motion detection
- Optimize frame latency
- Deploy to ARM-based production board
