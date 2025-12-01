const express = require('express');
const http = require('http');
const path = require('path');
const dgram = require('dgram');
const { Server } = require('socket.io');

const app = express();
const server = http.createServer(app);
const io = new Server(server);

const HTTP_PORT = 3000;

// UDP settings
const UDP_PORT = 12345;
const UDP_HOST = '0.0.0.0';
const TARGET_IP = '192.168.7.2'; // BeagleY board IP

// Serve static HTML/JS files from 'public'
app.use(express.static(path.join(__dirname, 'public')));

// Start HTTP server
server.listen(HTTP_PORT, () => {
    console.log(`Web server running on port ${HTTP_PORT}`);
});

// ---------------- UDP SOCKET ----------------
const udpSocket = dgram.createSocket('udp4');

udpSocket.on('listening', () => {
    const address = udpSocket.address();
    console.log(`UDP listening on ${address.address}:${address.port}`);
});

udpSocket.bind(UDP_PORT, UDP_HOST);

// ---------------- SOCKET.IO BUTTON CONTROL ----------------
io.on('connection', (socket) => {
    console.log('Browser connected for buttons:', socket.id);

    socket.on('servo', (cmd) => {
        // cmd should be "LEFT", "RIGHT", or "STOP"
        const message = Buffer.from(cmd);
        udpSocket.send(message, 0, message.length, UDP_PORT, TARGET_IP, (err) => {
            if (err) console.error('UDP send error:', err);
            else console.log(`Sent UDP command: ${cmd}`);
        });
    });

    socket.on('disconnect', () => {
        console.log('Browser disconnected for buttons:', socket.id);
    });
});
