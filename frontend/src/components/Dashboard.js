/**
 * Dashboard Component
 */

import React, { useState, useEffect } from 'react';
import APIClient from '../services/api';
import WebSocketClient from '../services/websocket';

function Dashboard() {
  const [hostInfo, setHostInfo] = useState(null);
  const [metrics, setMetrics] = useState(null);
  const [streams, setStreams] = useState([]);
  const [wsClient] = useState(() => new WebSocketClient());

  useEffect(() => {
    // Fetch initial data
    APIClient.getHostInfo()
      .then(setHostInfo)
      .catch(err => console.error('Failed to fetch host info:', err));

    APIClient.getCurrentMetrics()
      .then(setMetrics)
      .catch(err => console.error('Failed to fetch metrics:', err));

    APIClient.getStreams()
      .then(data => setStreams(data.streams || []))
      .catch(err => console.error('Failed to fetch streams:', err));

    // Connect WebSocket for real-time updates
    wsClient.connect()
      .then(() => {
        wsClient.subscribe('metrics', (message) => {
          if (message.data) {
            setMetrics(message.data);
          }
        });
      })
      .catch(err => console.error('WebSocket connection failed:', err));

    // Cleanup on unmount
    return () => {
      wsClient.disconnect();
    };
  }, [wsClient]);

  return (
    <div className="dashboard">
      <h1>RootStream Dashboard</h1>

      {/* System Status */}
      <section className="system-status">
        <h2>System Status</h2>
        {hostInfo ? (
          <div className="status-card">
            <p><strong>Hostname:</strong> {hostInfo.hostname}</p>
            <p><strong>Platform:</strong> {hostInfo.platform}</p>
            <p><strong>Version:</strong> {hostInfo.rootstream_version}</p>
            <p><strong>Uptime:</strong> {Math.floor(hostInfo.uptime_seconds / 3600)}h</p>
            <p><strong>Status:</strong> {hostInfo.is_streaming ? '🟢 Streaming' : '🔴 Idle'}</p>
          </div>
        ) : (
          <p>Loading system status...</p>
        )}
      </section>

      {/* Metrics Grid */}
      <section className="metrics-grid">
        <h2>Live Metrics</h2>
        {metrics ? (
          <div className="metrics-cards">
            <div className="metric-card">
              <h3>FPS</h3>
              <p className="metric-value">{metrics.fps}</p>
              <p className="metric-unit">frames/sec</p>
            </div>
            <div className="metric-card">
              <h3>Latency</h3>
              <p className="metric-value">{metrics.rtt_ms}</p>
              <p className="metric-unit">ms</p>
            </div>
            <div className="metric-card">
              <h3>GPU Usage</h3>
              <p className="metric-value">{metrics.gpu_util}%</p>
              <p className="metric-unit">{metrics.gpu_temp}°C</p>
            </div>
            <div className="metric-card">
              <h3>Bandwidth</h3>
              <p className="metric-value">{metrics.bandwidth_mbps.toFixed(1)}</p>
              <p className="metric-unit">Mbps</p>
            </div>
          </div>
        ) : (
          <p>Loading metrics...</p>
        )}
      </section>

      {/* Active Streams */}
      <section className="streams-section">
        <h2>Active Streams</h2>
        {streams.length > 0 ? (
          <div className="streams-list">
            {streams.map((stream) => (
              <div key={stream.stream_id} className="stream-item">
                <p><strong>{stream.peer_name}</strong></p>
                <p>{stream.width}x{stream.height} @ {stream.fps} FPS</p>
                {stream.is_recording && <span className="recording-badge">🔴 Recording</span>}
              </div>
            ))}
          </div>
        ) : (
          <p>No active streams</p>
        )}
      </section>
    </div>
  );
}

export default Dashboard;
