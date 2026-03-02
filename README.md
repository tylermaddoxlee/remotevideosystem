# remotevideosystem
## Overview:
This project implements a real-time embedded vision system capable of:

Live video streaming over UDP

Motion detection using OpenCV

Remote device control (servo motor, joystick input)

Web-based video relay via WebSockets

The system was developed in a four-person team and runs on Linux-based embedded platforms.

![image alt](https://github.com/tylermaddoxlee/remotevideosystem/blob/2aff2d9060c08fdea00ce91213d5d80cd99ec62e/project/ENSC351%20-%20Demo.webp)

## System Architecture:

Camera → OpenCV Processing → UDP Stream → WebSocket Relay → Browser Client

Hardware Interfaces:

SPI (MCP3208 ADC for joystick input)

PWM (Servo motor control)

GPIO (Rotary encoder button)

USB Webcam

Speaker output

![image alt](https://github.com/tylermaddoxlee/remotevideosystem/blob/2aff2d9060c08fdea00ce91213d5d80cd99ec62e/project/unnamed.png)

## Features

Real-time video capture and streaming

Motion detection pipeline using OpenCV

Remote control of servo motor via joystick

Hardware integration using SPI and PWM

Multithreaded data handling
