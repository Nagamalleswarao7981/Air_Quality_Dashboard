# Remote Backend for Air Quality Monitor

This Node.js application serves as the bridge between your ESP32 device (via MQTT) and the Web Dashboard (via WebSockets).

## Prerequisites
- Node.js installed
- A public MQTT Broker (e.g., HiveMQ Cloud, bare EMQX, or `broker.emqx.io` for testing)

## Setup
1. Connection Configuration:
   Create a `.env` file in this directory:
   ```env
   PORT=3000
   MQTT_BROKER=mqtt://broker.uniolabs.in
   MQTT_USER=
   MQTT_PASS=
   ```

2. Install Dependencies:
   ```bash
   npm install
   ```

3. Run Server:
   ```bash
   npm start
   ```

## Deployment Options (Free)
1. **Render.com / Railway.app / Fly.io**:
   - Push this `remote_backend` folder to a GitHub repository.
   - Connect it to one of these services.
   - Set the Environment Variables in their dashboard.

2. **Replit**:
   - Import this code into a Node.js repl.
   - Hit 'Run'.

## Dashboard
Access the dashboard at `http://localhost:3000` (or your deployed URL).
It will automatically show devices as they publish data.
