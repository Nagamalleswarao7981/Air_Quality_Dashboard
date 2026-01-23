require('dotenv').config();
const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const mqtt = require('mqtt');
const path = require('path');
const multer = require('multer');
const fs = require('fs');

// Ensure uploads dir
const uploadDir = path.join(__dirname, 'public/uploads');
if (!fs.existsSync(uploadDir)) {
    fs.mkdirSync(uploadDir, { recursive: true });
}

// Storage for firmware
const storage = multer.diskStorage({
    destination: function (req, file, cb) {
        cb(null, uploadDir)
    },
    filename: function (req, file, cb) {
        cb(null, 'firmware-' + Date.now() + path.extname(file.originalname))
    }
})
const upload = multer({ storage: storage });

// --- Configuration ---
const PORT = process.env.PORT || 3000;
const MQTT_BROKER = process.env.MQTT_BROKER || 'mqtt://broker.uniolabs.in';
const MQTT_USER = process.env.MQTT_USER || '';
const MQTT_PASS = process.env.MQTT_PASS || '';

// --- Setup ---
const app = express();
const server = http.createServer(app);
const io = new Server(server);

// Serve static frontend
// Serve static frontend
app.use(express.static(path.join(__dirname, 'public')));
app.use(express.json()); // Enable JSON body parsing

// --- API Routes ---
// 1. Upload Firmware
app.post('/api/ota/upload', upload.single('firmware'), (req, res) => {
    if (!req.file) return res.status(400).send('No file uploaded.');

    // Calculate simple hash if needed, or just return URL
    const fileUrl = `${req.protocol}://${req.get('host')}/uploads/${req.file.filename}`;
    res.json({ url: fileUrl, filename: req.file.filename, size: req.file.size });
});

// 2. Start OTA
app.post('/api/ota/start', (req, res) => {
    const { deviceId, fileUrl, version } = req.body;
    if (!deviceId || !fileUrl) return res.status(400).send('Missing params');

    const topic = `aqm/${deviceId}/cmd`;
    const payload = JSON.stringify({
        cmd: "ota_start",
        url: fileUrl,
        ver: version || "unknown"
    });

    mqttClient.publish(topic, payload, { qos: 1 });
    console.log(`[OTA] Sent ota_start to ${deviceId}: ${fileUrl}`);
    res.json({ status: 'command_sent' });
});

// --- MQTT Client ---
console.log(`[MQTT] Connecting to ${MQTT_BROKER}...`);
const mqttClient = mqtt.connect(MQTT_BROKER, {
    username: MQTT_USER,
    password: MQTT_PASS
});

mqttClient.on('connect', () => {
    console.log('[MQTT] Connected');
    // Subscribe to all device data
    mqttClient.subscribe('aqm/+/data', (err) => {
        if (!err) console.log('[MQTT] Subscribed to aqm/+/data');
    });
    mqttClient.subscribe('aqm/+/status', (err) => {
        if (!err) console.log('[MQTT] Subscribed to aqm/+/status');
    });
    mqttClient.subscribe('aqm/+/ota_status', (err) => {
        if (!err) console.log('[MQTT] Subscribed to aqm/+/ota_status');
    });
});

mqttClient.on('message', (topic, message) => {
    // Topic format: aqm/<device_id>/data OR aqm/<device_id>/status
    const parts = topic.split('/');
    if (parts.length === 3) {
        const deviceId = parts[1];
        const type = parts[2]; // 'data' or 'status'

        try {
            const payload = JSON.parse(message.toString());

            if (type === 'data') {
                io.emit('device_data', { deviceId, data: payload });
            } else if (type === 'status') {
                io.emit('device_status', { deviceId, status: payload });
            } else if (type === 'ota_status') {
                io.emit('ota_progress', { deviceId, status: payload });
            }
        } catch (e) {
            console.error('[MQTT] JSON Parse Error:', e);
        }
    }
});

// --- WebSocket Handling ---
io.on('connection', (socket) => {
    console.log('[WS] Client connected');

    socket.on('command', (msg) => {
        // msg: { deviceId, command, payload }
        // Topic: aqm/<device_id>/cmd
        if (msg.deviceId && msg.command) {
            const topic = `aqm/${msg.deviceId}/cmd`;
            const payload = JSON.stringify({ cmd: msg.command, val: msg.payload });

            mqttClient.publish(topic, payload);
            console.log(`[CMD] Sent to ${topic}: ${payload}`);
        }
    });

    socket.on('disconnect', () => {
        console.log('[WS] Client disconnected');
    });
});

// --- Start Server ---
server.listen(PORT, () => {
    console.log(`[Server] Running on http://localhost:${PORT}`);
});
