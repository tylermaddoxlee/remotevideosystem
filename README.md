# remotevideosystem
## Overview:
This project implements a real-time embedded vision system capable of:

Live video streaming over UDP

Motion detection using OpenCV

Remote device control (servo motor, joystick input)

Web-based video relay via WebSockets

The system was developed in a four-person team and runs on Linux-based embedded platforms.
![image alt]((https://github.com/tylermaddoxlee/remotevideosystem/blob/eb2d653717def80a179cffe70a0efb03398a8d35/project/remote.png)) 
## System Architecture:

Camera → OpenCV Processing → UDP Stream → WebSocket Relay → Browser Client

Hardware Interfaces:

SPI (MCP3208 ADC for joystick input)

PWM (Servo motor control)

GPIO (Rotary encoder button)

USB Webcam

Speaker output


## Features

Real-time video capture and streaming

Motion detection pipeline using OpenCV

Remote control of servo motor via joystick

Hardware integration using SPI and PWM

Multithreaded data handling
