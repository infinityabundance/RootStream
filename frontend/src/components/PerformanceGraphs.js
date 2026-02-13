/**
 * Performance Graphs Component
 */

import React, { useState, useEffect } from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import WebSocketClient from '../services/websocket';

function PerformanceGraphs() {
  const [fpsData, setFpsData] = useState([]);
  const [latencyData, setLatencyData] = useState([]);
  const [gpuData, setGpuData] = useState([]);
  const [wsClient] = useState(() => new WebSocketClient());

  useEffect(() => {
    // Connect WebSocket for real-time updates
    wsClient.connect()
      .then(() => {
        wsClient.subscribe('metrics', (message) => {
          if (message.data) {
            const timestamp = new Date().toLocaleTimeString();
            const metrics = message.data;

            // Update FPS data (keep last 60 samples)
            setFpsData(prev => [...prev.slice(-59), {
              time: timestamp,
              fps: metrics.fps
            }]);

            // Update latency data
            setLatencyData(prev => [...prev.slice(-59), {
              time: timestamp,
              rtt: metrics.rtt_ms,
              jitter: metrics.jitter_ms
            }]);

            // Update GPU data
            setGpuData(prev => [...prev.slice(-59), {
              time: timestamp,
              utilization: metrics.gpu_util,
              temp: metrics.gpu_temp
            }]);
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
    <div className="performance-graphs">
      <h1>Performance Monitoring</h1>

      <div className="graph-container">
        <h2>Frame Rate (FPS)</h2>
        <ResponsiveContainer width="100%" height={300}>
          <LineChart data={fpsData}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="time" />
            <YAxis />
            <Tooltip />
            <Line type="monotone" dataKey="fps" stroke="#8884d8" dot={false} />
          </LineChart>
        </ResponsiveContainer>
      </div>

      <div className="graph-container">
        <h2>Network Latency</h2>
        <ResponsiveContainer width="100%" height={300}>
          <LineChart data={latencyData}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="time" />
            <YAxis />
            <Tooltip />
            <Legend />
            <Line type="monotone" dataKey="rtt" stroke="#82ca9d" name="RTT (ms)" dot={false} />
            <Line type="monotone" dataKey="jitter" stroke="#ffc658" name="Jitter (ms)" dot={false} />
          </LineChart>
        </ResponsiveContainer>
      </div>

      <div className="graph-container">
        <h2>GPU Metrics</h2>
        <ResponsiveContainer width="100%" height={300}>
          <LineChart data={gpuData}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="time" />
            <YAxis />
            <Tooltip />
            <Legend />
            <Line type="monotone" dataKey="utilization" stroke="#ff7c7c" name="Utilization (%)" dot={false} />
            <Line type="monotone" dataKey="temp" stroke="#ffc658" name="Temperature (°C)" dot={false} />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}

export default PerformanceGraphs;
