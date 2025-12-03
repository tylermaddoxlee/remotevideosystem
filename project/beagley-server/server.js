// server.js
const express = require('express');
const http = require('http');
const path = require('path');
const dgram = require('dgram');
const net = require('net');
const fs = require('fs');
const { Server } = require('socket.io');

const app = express();
const server = http.createServer(app);
const io = new Server(server);

const HTTP_PORT = 3000;

// UDP
// Shared UDP port for sampler *and* joystick commands
const UDP_PORT = 12345;
const UDP_HOST = '0.0.0.0';
const TARGET_IP = '192.168.7.2'; // BeagleY board IP for joystick commands

// Motion events from Beagle (C++ motion server)
const MOTION_PORT = 12346;
const MOTION_HOST = '0.0.0.0';

// BeagleY camera + audio
const BEAGLEY_IP = "192.168.7.2";
const CAMERA_PORT = 8554;
const AUDIO_PORT  = 8555;

// Clips directory (where OpenCV saves clips)
const CLIPS_DIR = path.join(__dirname, 'clips');
// const CLIPS_DIR = "/home/ariful/ensc351/public/project/beagley-server/clips";

if (!fs.existsSync(CLIPS_DIR)) {
  fs.mkdirSync(CLIPS_DIR, { recursive: true });
  console.log(`Created clips directory at ${CLIPS_DIR}`);
}
console.log("Using CLIPS_DIR =", CLIPS_DIR);

//NEWEST CLIPS
function pruneOldClips(maxClips = 5) {
  try {
    // Get all video files
    const files = fs.readdirSync(CLIPS_DIR)
      .filter(f => f.endsWith('.avi') || f.endsWith('.mp4'));

    if (files.length <= maxClips) {
      return;
    }

    // Build array with mtime for each file
    const withTimes = files.map(name => {
      const full = path.join(CLIPS_DIR, name);
      const stat = fs.statSync(full);
      return { name, mtime: stat.mtimeMs };
    });

    // Oldest first
    withTimes.sort((a, b) => a.mtime - b.mtime);

    const extraCount = withTimes.length - maxClips;
    const toDelete = withTimes.slice(0, extraCount);

    toDelete.forEach(file => {
      const full = path.join(CLIPS_DIR, file.name);
      try {
        fs.unlinkSync(full);
        console.log(`Pruned old clip: ${file.name}`);
      } catch (err) {
        console.error(`Failed to delete clip ${file.name}:`, err);
      }
    });
  } catch (err) {
    console.error("Error pruning clips:", err);
  }
}

// STATIC FILES
app.use(express.static(path.join(__dirname, 'public')));
app.use('/clips', express.static(CLIPS_DIR));

// Socket
io.on('connection', (socket) => {
  console.log('Browser connected:', socket.id);

  // Joystick buttons -> UDP to Beagle
  socket.on('servo', (cmd) => {
    // cmd: "LEFT", "RIGHT", "STOP"
    const message = Buffer.from(cmd);
    udpSocket.send(message, 0, message.length, UDP_PORT, TARGET_IP, (err) => {
      if (err) console.error('UDP send error (servo):', err);
      else     console.log(`Sent UDP servo command: ${cmd}`);
    });
  });

  socket.on('disconnect', () => {
    console.log('Browser disconnected:', socket.id);
  });
});

// Camera Stream
app.get('/camera', (req, res) => {
  res.writeHead(200, {
    'Content-Type': 'multipart/x-mixed-replace; boundary=frame'
  });

  const client = net.createConnection(
    { host: BEAGLEY_IP, port: CAMERA_PORT },
    () => console.log("Connected to BeagleY camera stream")
  );

  client.on('data', (chunk) => res.write(chunk));

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

// Audio Stream
app.get('/audio', (req, res) => {
  res.writeHead(200, {
    'Content-Type': 'audio/mpeg',
    'Connection': 'close',
    'Transfer-Encoding': 'chunked'
  });

  const socket = net.connect(AUDIO_PORT, BEAGLEY_IP, () => {
    console.log('Connected to audio stream');
  });

  socket.on('data', (chunk) => res.write(chunk));

  socket.on('end', () => {
    console.log('Audio stream ended');
    res.end();
  });

  socket.on('error', (err) => {
    console.error('Audio socket error:', err);
    res.end();
  });

  req.on('close', () => {
    console.log('Browser closed /audio');
    socket.end();
  });
});

// UDP Sample
const udpSocket = dgram.createSocket('udp4');

udpSocket.on('listening', () => {
  const address = udpSocket.address();
  console.log(`UDP listening on ${address.address}:${address.port}`);
});

// If you still use this for sampler logs:
udpSocket.on('message', (msg, rinfo) => {
  const text = msg.toString().trim();
  console.log(`UDP ${rinfo.address}:${rinfo.port} --> ${text}`);
  io.emit('sample', text);
});

udpSocket.bind(UDP_PORT, UDP_HOST);

// Motion Events
const motionSocket = dgram.createSocket('udp4');

motionSocket.on('listening', () => {
  const address = motionSocket.address();
  console.log(`MOTION UDP listening on ${address.address}:${address.port}`);
});

motionSocket.on('message', (msg, rinfo) => {
  const text = msg.toString().trim();
  console.log(`MOTION from ${rinfo.address}:${rinfo.port} --> ${text}`);

  // Broadcast to browser
  io.emit('motion', { text, ts: Date.now() });
});

motionSocket.bind(MOTION_PORT, MOTION_HOST);

// Clips
app.get('/api/clips', (req, res) => {
  console.log("GET /api/clips");

  // Housekeeping: keep only 5 newest
  pruneOldClips(5);

  // Now read the (possibly pruned) directory
  fs.readdir(CLIPS_DIR, (err, files) => {
    if (err) {
      console.error("Error reading clips dir:", err);
      return res.status(500).json({ error: 'Failed to list clips' });
    }

    const clips = files
      .filter(f => f.endsWith('.avi') || f.endsWith('.mp4'))
      .sort()
      .reverse();  // newest names last -> reverse for newest first

    res.json(clips);
  });
});

// Folder style visuals
app.get('/clips-browser', (req, res) => {
  console.log("GET /clips-browser");

  // Also prune here, in case this is hit first
  pruneOldClips(5);

  fs.readdir(CLIPS_DIR, (err, files) => {
    if (err) {
      console.error("Error reading clips dir (browser):", err);
      return res
        .status(500)
        .send('<h1>Error reading clips directory</h1>');
    }

    const list = files
      .filter(f => f.endsWith('.avi') || f.endsWith('.mp4'))
      .sort()
      .reverse()
      .map(f => `<li><a href="/clips/${encodeURIComponent(f)}">${f}</a></li>`)
      .join('\n');

    res.send(`
      <!DOCTYPE html>
      <html>
      <head>
        <meta charset="utf-8">
        <title>Clips Folder View</title>
        <style>
          body { background:#111; color:#eee; font-family:Arial,sans-serif; }
          a { color:#6cf; }
        </style>
      </head>
      <body>
        <h1>Clips Folder View</h1>
        <ul>${list}</ul>
      </body>
      </html>
    `);
  });
});

// START SERVER
server.listen(HTTP_PORT, '0.0.0.0', () => {
  console.log(`Web server running on port ${HTTP_PORT}`);
});
