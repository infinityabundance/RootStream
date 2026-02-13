//
//  StreamingClient.swift
//  RootStream iOS
//
//  Network streaming client using NWConnection with TLS
//

import Foundation
import Network

@MainActor
class StreamingClient: ObservableObject {
    @Published var isConnected = false
    @Published var currentFPS: Double = 0
    @Published var currentLatency: Int = 0
    
    var renderer: MetalRenderer?
    
    private var connection: NWConnection?
    private var videoDecoder: VideoDecoder?
    private var audioEngine: AudioEngine?
    private var receiveQueue = DispatchQueue(label: "com.rootstream.receive")
    private var sendQueue = DispatchQueue(label: "com.rootstream.send")
    
    private var sequenceNumber: UInt32 = 0
    private var lastFrameTime: Date?
    private var frameCount: Int = 0
    
    init() {
        self.renderer = MetalRenderer()
        self.videoDecoder = VideoDecoder()
        self.audioEngine = AudioEngine()
    }
    
    func connect(to peer: Peer) async throws {
        let host = NWEndpoint.Host(peer.hostname)
        let port = NWEndpoint.Port(rawValue: peer.port) ?? .init(integerLiteral: 8000)
        
        // Create TLS options for secure connection
        let tlsOptions = NWProtocolTLS.Options()
        
        // Configure TCP options
        let tcpOptions = NWProtocolTCP.Options()
        tcpOptions.noDelay = true // Disable Nagle's algorithm for low latency
        
        let parameters = NWParameters(tls: tlsOptions, tcp: tcpOptions)
        parameters.includePeerToPeer = true
        
        connection = NWConnection(host: host, port: port, using: parameters)
        
        return try await withCheckedThrowingContinuation { continuation in
            connection?.stateUpdateHandler = { [weak self] state in
                Task { @MainActor in
                    guard let self = self else { return }
                    
                    switch state {
                    case .ready:
                        self.isConnected = true
                        self.startReceiveLoop()
                        continuation.resume()
                    case .failed(let error):
                        self.isConnected = false
                        continuation.resume(throwing: error)
                    case .cancelled:
                        self.isConnected = false
                    default:
                        break
                    }
                }
            }
            
            connection?.start(queue: receiveQueue)
        }
    }
    
    func disconnect() {
        connection?.cancel()
        connection = nil
        isConnected = false
    }
    
    func sendInput(_ event: InputEvent) {
        guard let connection = connection else { return }
        
        let packet = StreamPacket(
            type: .inputEvent,
            timestamp: UInt64(Date().timeIntervalSince1970 * 1000),
            sequenceNumber: sequenceNumber,
            data: event.data
        )
        
        sequenceNumber += 1
        
        connection.send(content: packet.serialized, completion: .contentProcessed { error in
            if let error = error {
                print("Send error: \(error)")
            }
        })
    }
    
    private func startReceiveLoop() {
        receivePacket()
    }
    
    private func receivePacket() {
        connection?.receive(minimumIncompleteLength: 1, maximumLength: 65536) { [weak self] data, _, isComplete, error in
            guard let self = self else { return }
            
            if let data = data, !data.isEmpty {
                Task { @MainActor in
                    self.handleReceivedData(data)
                }
            }
            
            if let error = error {
                print("Receive error: \(error)")
                Task { @MainActor in
                    self.isConnected = false
                }
                return
            }
            
            if !isComplete {
                self.receivePacket()
            }
        }
    }
    
    private func handleReceivedData(_ data: Data) {
        guard let packet = StreamPacket.deserialize(data) else {
            print("Failed to deserialize packet")
            return
        }
        
        switch packet.type {
        case .videoFrame:
            handleVideoFrame(packet.data)
            updateFPS()
        case .audioFrame:
            handleAudioFrame(packet.data)
        case .keepAlive:
            // Send keepalive response
            break
        default:
            break
        }
        
        // Calculate latency
        let packetTime = Date(timeIntervalSince1970: TimeInterval(packet.timestamp) / 1000)
        currentLatency = Int(Date().timeIntervalSince(packetTime) * 1000)
    }
    
    private func handleVideoFrame(_ data: Data) {
        videoDecoder?.decode(data) { [weak self] pixelBuffer in
            guard let self = self, let pixelBuffer = pixelBuffer else { return }
            Task { @MainActor in
                self.renderer?.renderFrame(pixelBuffer)
            }
        }
    }
    
    private func handleAudioFrame(_ data: Data) {
        audioEngine?.playAudioData(data)
    }
    
    private func updateFPS() {
        let now = Date()
        frameCount += 1
        
        if let lastTime = lastFrameTime {
            let elapsed = now.timeIntervalSince(lastTime)
            if elapsed >= 1.0 {
                currentFPS = Double(frameCount) / elapsed
                frameCount = 0
                lastFrameTime = now
            }
        } else {
            lastFrameTime = now
        }
    }
}
