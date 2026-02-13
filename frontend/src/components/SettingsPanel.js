/**
 * Settings Panel Component
 */

import React, { useState, useEffect } from 'react';
import APIClient from '../services/api';

function SettingsPanel() {
  const [videoSettings, setVideoSettings] = useState({});
  const [audioSettings, setAudioSettings] = useState({});
  const [networkSettings, setNetworkSettings] = useState({});
  const [saveStatus, setSaveStatus] = useState('');

  useEffect(() => {
    // Fetch current settings
    Promise.all([
      APIClient.getVideoSettings().then(setVideoSettings),
      APIClient.getAudioSettings().then(setAudioSettings),
      APIClient.getNetworkSettings().then(setNetworkSettings)
    ]).catch(err => console.error('Failed to fetch settings:', err));
  }, []);

  const handleSaveVideoSettings = async () => {
    try {
      await APIClient.updateVideoSettings(videoSettings);
      setSaveStatus('Video settings saved ✓');
      setTimeout(() => setSaveStatus(''), 3000);
    } catch (error) {
      setSaveStatus('Error saving video settings');
      console.error('Failed to save video settings:', error);
    }
  };

  const handleSaveAudioSettings = async () => {
    try {
      await APIClient.updateAudioSettings(audioSettings);
      setSaveStatus('Audio settings saved ✓');
      setTimeout(() => setSaveStatus(''), 3000);
    } catch (error) {
      setSaveStatus('Error saving audio settings');
      console.error('Failed to save audio settings:', error);
    }
  };

  const handleSaveNetworkSettings = async () => {
    try {
      await APIClient.updateNetworkSettings(networkSettings);
      setSaveStatus('Network settings saved ✓');
      setTimeout(() => setSaveStatus(''), 3000);
    } catch (error) {
      setSaveStatus('Error saving network settings');
      console.error('Failed to save network settings:', error);
    }
  };

  return (
    <div className="settings-panel">
      <h1>Settings</h1>

      {saveStatus && <div className="status-message">{saveStatus}</div>}

      {/* Video Settings */}
      <section className="settings-section">
        <h2>Video Settings</h2>
        <div className="settings-form">
          <label>
            Resolution:
            <select 
              value={`${videoSettings.width}x${videoSettings.height}`} 
              onChange={(e) => {
                const [width, height] = e.target.value.split('x').map(Number);
                setVideoSettings({...videoSettings, width, height});
              }}
            >
              <option value="1280x720">1280x720 (720p)</option>
              <option value="1920x1080">1920x1080 (1080p)</option>
              <option value="2560x1440">2560x1440 (1440p)</option>
              <option value="3840x2160">3840x2160 (4K)</option>
            </select>
          </label>
          <label>
            FPS:
            <input 
              type="number" 
              value={videoSettings.fps || 60}
              onChange={(e) => setVideoSettings({...videoSettings, fps: parseInt(e.target.value)})}
              min="30" 
              max="120" 
              step="10"
            />
          </label>
          <label>
            Bitrate (kbps):
            <input 
              type="number" 
              value={videoSettings.bitrate_kbps || 20000}
              onChange={(e) => setVideoSettings({...videoSettings, bitrate_kbps: parseInt(e.target.value)})}
              min="5000" 
              max="50000" 
              step="1000"
            />
          </label>
          <label>
            Encoder:
            <select 
              value={videoSettings.encoder || 'vaapi'}
              onChange={(e) => setVideoSettings({...videoSettings, encoder: e.target.value})}
            >
              <option value="vaapi">VA-API</option>
              <option value="nvenc">NVENC</option>
              <option value="ffmpeg">FFmpeg</option>
              <option value="raw">Raw</option>
            </select>
          </label>
          <button onClick={handleSaveVideoSettings}>Save Video Settings</button>
        </div>
      </section>

      {/* Audio Settings */}
      <section className="settings-section">
        <h2>Audio Settings</h2>
        <div className="settings-form">
          <label>
            Output Device:
            <select 
              value={audioSettings.output_device || 'default'}
              onChange={(e) => setAudioSettings({...audioSettings, output_device: e.target.value})}
            >
              <option value="default">Default</option>
              <option value="headphones">Headphones</option>
              <option value="speakers">Speakers</option>
            </select>
          </label>
          <label>
            Sample Rate:
            <select 
              value={audioSettings.sample_rate || 48000}
              onChange={(e) => setAudioSettings({...audioSettings, sample_rate: parseInt(e.target.value)})}
            >
              <option value="44100">44.1 kHz</option>
              <option value="48000">48 kHz</option>
            </select>
          </label>
          <label>
            Channels:
            <select 
              value={audioSettings.channels || 2}
              onChange={(e) => setAudioSettings({...audioSettings, channels: parseInt(e.target.value)})}
            >
              <option value="1">Mono</option>
              <option value="2">Stereo</option>
            </select>
          </label>
          <button onClick={handleSaveAudioSettings}>Save Audio Settings</button>
        </div>
      </section>

      {/* Network Settings */}
      <section className="settings-section">
        <h2>Network Settings</h2>
        <div className="settings-form">
          <label>
            Port:
            <input 
              type="number" 
              value={networkSettings.port || 9090}
              onChange={(e) => setNetworkSettings({...networkSettings, port: parseInt(e.target.value)})}
              min="1024" 
              max="65535"
            />
          </label>
          <label>
            Target Bitrate (Mbps):
            <input 
              type="number" 
              value={networkSettings.target_bitrate_mbps || 25}
              onChange={(e) => setNetworkSettings({...networkSettings, target_bitrate_mbps: parseInt(e.target.value)})}
              min="5" 
              max="100" 
              step="5"
            />
          </label>
          <label>
            <input 
              type="checkbox" 
              checked={networkSettings.enable_tcp_fallback || false}
              onChange={(e) => setNetworkSettings({...networkSettings, enable_tcp_fallback: e.target.checked})}
            />
            Enable TCP Fallback
          </label>
          <label>
            <input 
              type="checkbox" 
              checked={networkSettings.enable_encryption || false}
              onChange={(e) => setNetworkSettings({...networkSettings, enable_encryption: e.target.checked})}
            />
            Enable Encryption
          </label>
          <button onClick={handleSaveNetworkSettings}>Save Network Settings</button>
        </div>
      </section>
    </div>
  );
}

export default SettingsPanel;
