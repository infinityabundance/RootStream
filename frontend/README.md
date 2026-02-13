# RootStream Web Dashboard

Real-time web-based dashboard for monitoring and managing RootStream streaming sessions.

## Features

- **Real-time Metrics**: Live FPS, latency, GPU/CPU usage, network stats
- **System Monitoring**: Host information and status
- **Performance Graphs**: Historical data visualization with charts
- **Settings Management**: Configure video, audio, and network settings
- **WebSocket Updates**: Real-time data push from server
- **Responsive Design**: Works on desktop and mobile devices

## Development

### Prerequisites

- Node.js 16+ and npm
- RootStream backend running on localhost:8080 (REST API)
- WebSocket server on localhost:8081

### Installation

```bash
cd frontend
npm install
```

### Running Development Server

```bash
npm start
```

This will start the development server on http://localhost:3000

### Building for Production

```bash
npm run build
```

This creates an optimized production build in the `build/` directory.

## API Endpoints

### Host
- `GET /api/host/info` - Get host information
- `POST /api/host/start` - Start streaming host
- `POST /api/host/stop` - Stop streaming host

### Metrics
- `GET /api/metrics/current` - Get current metrics
- `GET /api/metrics/history` - Get metrics history

### Streams
- `GET /api/streams` - Get active streams
- `POST /api/streams/:id/record` - Start recording
- `POST /api/streams/:id/stop-record` - Stop recording

### Settings
- `GET /api/settings/video` - Get video settings
- `PUT /api/settings/video` - Update video settings
- `GET /api/settings/audio` - Get audio settings
- `PUT /api/settings/audio` - Update audio settings
- `GET /api/settings/network` - Get network settings
- `PUT /api/settings/network` - Update network settings

### Authentication
- `POST /api/auth/login` - Login (get JWT token)
- `POST /api/auth/logout` - Logout
- `GET /api/auth/verify` - Verify token

## WebSocket Messages

### Metrics Update
```json
{
  "type": "metrics",
  "data": {
    "fps": 60,
    "rtt_ms": 15,
    "gpu_util": 45,
    ...
  }
}
```

### Event Notification
```json
{
  "type": "event",
  "event_type": "stream_started",
  "data": "..."
}
```

## Architecture

```
frontend/
├── public/              # Static files
│   └── index.html
├── src/
│   ├── components/      # React components
│   │   ├── Dashboard.js
│   │   ├── PerformanceGraphs.js
│   │   ├── SettingsPanel.js
│   │   └── Navbar.js
│   ├── services/        # API clients
│   │   ├── api.js       # REST API client
│   │   └── websocket.js # WebSocket client
│   ├── styles/          # CSS styles
│   │   └── App.css
│   ├── App.js           # Main App component
│   └── index.js         # Entry point
└── package.json
```

## Technology Stack

- **React 18** - UI framework
- **Recharts** - Data visualization
- **WebSocket API** - Real-time updates
- **Fetch API** - REST API communication

## License

MIT License - see parent project LICENSE file
