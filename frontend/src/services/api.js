/**
 * API Client Service - REST API communication
 */

const API_BASE_URL = '/api';

class APIClient {
  static getAuthToken() {
    return localStorage.getItem('authToken');
  }

  static setAuthToken(token) {
    localStorage.setItem('authToken', token);
  }

  static clearAuthToken() {
    localStorage.removeItem('authToken');
  }

  static async request(method, endpoint, body = null) {
    const url = `${API_BASE_URL}${endpoint}`;
    const headers = {
      'Content-Type': 'application/json',
    };

    const token = this.getAuthToken();
    if (token) {
      headers['Authorization'] = `Bearer ${token}`;
    }

    const options = {
      method,
      headers,
    };

    if (body) {
      options.body = JSON.stringify(body);
    }

    try {
      const response = await fetch(url, options);

      if (response.status === 401) {
        // Token expired, redirect to login
        this.clearAuthToken();
        window.location.href = '/login';
        throw new Error('Unauthorized');
      }

      if (!response.ok) {
        throw new Error(`API Error: ${response.statusText}`);
      }

      return response.json();
    } catch (error) {
      console.error('API request failed:', error);
      throw error;
    }
  }

  // Authentication
  static async login(username, password) {
    const response = await this.request('POST', '/auth/login', { username, password });
    if (response.token) {
      this.setAuthToken(response.token);
    }
    return response;
  }

  static async logout() {
    await this.request('POST', '/auth/logout');
    this.clearAuthToken();
  }

  static async verifyAuth() {
    return this.request('GET', '/auth/verify');
  }

  // Host
  static async getHostInfo() {
    return this.request('GET', '/host/info');
  }

  static async startHost() {
    return this.request('POST', '/host/start');
  }

  static async stopHost() {
    return this.request('POST', '/host/stop');
  }

  // Metrics
  static async getCurrentMetrics() {
    return this.request('GET', '/metrics/current');
  }

  static async getMetricsHistory() {
    return this.request('GET', '/metrics/history');
  }

  // Peers
  static async getPeers() {
    return this.request('GET', '/peers');
  }

  // Streams
  static async getStreams() {
    return this.request('GET', '/streams');
  }

  static async startRecording(streamId) {
    return this.request('POST', `/streams/${streamId}/record`);
  }

  static async stopRecording(streamId) {
    return this.request('POST', `/streams/${streamId}/stop-record`);
  }

  // Settings
  static async getVideoSettings() {
    return this.request('GET', '/settings/video');
  }

  static async updateVideoSettings(settings) {
    return this.request('PUT', '/settings/video', settings);
  }

  static async getAudioSettings() {
    return this.request('GET', '/settings/audio');
  }

  static async updateAudioSettings(settings) {
    return this.request('PUT', '/settings/audio', settings);
  }

  static async getNetworkSettings() {
    return this.request('GET', '/settings/network');
  }

  static async updateNetworkSettings(settings) {
    return this.request('PUT', '/settings/network', settings);
  }
}

export default APIClient;
