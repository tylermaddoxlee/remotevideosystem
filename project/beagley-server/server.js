const express = require('express');
const http = require('http');
const path = require('path');
const dgram = require('dgram');
const net = require('net');
const { Server } = require('socket.io');

const app = express();
const server = http.createServer(app);
const io = new Server(server);

const HTTP_PORT = 3000;

// UDP settings
const UDP_PORT = 12345;
const UDP_HOST = '0.0.0.0';

// BeagleY camera
const BEAGLEY_IP = "192.168.7.2";
const CAMERA_PORT = 8554;

// BeagleY audio (mic)
const AUDIO_PORT = 8555;

app.use(express.static(path.join(__dirname, 'public')));

io.on('connection', (socket) => {
  console.log('Browser connected:', socket.id);
  socket.on('disconnect', () => {
    console.log('Browser disconnected:', socket.id);
  });
});

// ---------------- CAMERA STREAM ----------------

app.get('/camera', (req, res) => {
  console.log("Browser requested /camera");

  res.writeHead(200, {
    'Content-Type': 'multipart/x-mixed-replace; boundary=frame'
  });

  const client = net.createConnection(
    { host: BEAGLEY_IP, port: CAMERA_PORT },
    () => console.log("Connected to BeagleY camera stream")
  );

  client.on('data', (chunk) => {
    console.log("Got", chunk.length, "bytes from camera");
    res.write(chunk);
  });

  req.on('close', () => {
    console.log("Browser closed /camera");
    client.end();
  });

  client.on('error', (err) => {
    console.error("Camera stream error:", err);
    res.end();
  });

  client.on('end', () => {
    console.log("Camera stream ended");
    res.end();
  });
});


// ---------------- AUDIO STREAM ----------------
app.get('/audio', (req, res) => {
  // Tell browser this is an MP3 audio stream
  res.writeHead(200, {
    'Content-Type': 'audio/mpeg',
    'Connection': 'close',
    'Transfer-Encoding': 'chunked'
  });

  // Connect to BeagleY audio TCP server
  const socket = net.connect(AUDIO_PORT, BEAGLEY_IP, () => {
    console.log('Connected to audio stream');
  });

  socket.on('data', (chunk) => {
    res.write(chunk);
  });

  socket.on('end', () => {
    console.log('Audio stream ended');
    res.end();
  });

  socket.on('error', (err) => {
    console.error('Audio socket error:', err);
    res.end();
  });

  // If browser closes tab / stops request
  req.on('close', () => {
    console.log('Browser closed /audio');
    socket.end();
  });
});

// ---------------- UDP SAMPLE LOGGING ----------------
const udpSocket = dgram.createSocket('udp4');

udpSocket.on('listening', () => {
  const address = udpSocket.address();
  console.log(`UDP listening on ${address.address}:${address.port}`);
});

udpSocket.on('message', (msg, rinfo) => {
  const text = msg.toString().trim();
  console.log(`UDP ${rinfo.address}:${rinfo.port} --> ${text}`);
  io.emit('sample', text);
});

udpSocket.bind(UDP_PORT, UDP_HOST);

server.listen(HTTP_PORT, '0.0.0.0', () => {
  console.log(`Web server running on port ${HTTP_PORT}`);
});
