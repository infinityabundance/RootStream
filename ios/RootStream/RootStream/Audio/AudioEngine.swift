//
//  AudioEngine.swift
//  RootStream iOS
//
//  Audio playback engine with Opus decoding
//

import Foundation
import AVFoundation

class AudioEngine {
    private var audioEngine: AVAudioEngine
    private var playerNode: AVAudioPlayerNode
    private var opusDecoder: OpusDecoder
    
    private let sampleRate: Double = 48000
    private let channels: UInt32 = 2
    
    init() {
        audioEngine = AVAudioEngine()
        playerNode = AVAudioPlayerNode()
        opusDecoder = OpusDecoder(sampleRate: Int(sampleRate), channels: Int(channels))
        
        setupAudioEngine()
    }
    
    private func setupAudioEngine() {
        // Attach player node
        audioEngine.attach(playerNode)
        
        // Configure audio format
        let format = AVAudioFormat(
            commonFormat: .pcmFormatFloat32,
            sampleRate: sampleRate,
            channels: AVAudioChannelCount(channels),
            interleaved: false
        )!
        
        // Connect player to output
        audioEngine.connect(playerNode, to: audioEngine.mainMixerNode, format: format)
        
        // Configure audio session
        configureAudioSession()
        
        // Start engine
        do {
            try audioEngine.start()
            playerNode.play()
        } catch {
            print("Failed to start audio engine: \(error)")
        }
    }
    
    private func configureAudioSession() {
        let audioSession = AVAudioSession.sharedInstance()
        
        do {
            // Set category for playback with low latency
            try audioSession.setCategory(.playback, mode: .default, options: [.mixWithOthers])
            
            // Set preferred buffer duration for low latency
            try audioSession.setPreferredIOBufferDuration(0.005) // 5ms
            
            // Activate session
            try audioSession.setActive(true)
        } catch {
            print("Failed to configure audio session: \(error)")
        }
    }
    
    func playAudioData(_ data: Data) {
        // Parse audio frame
        // Format: [sample_rate: 4 bytes][channels: 1 byte][opus_data: rest]
        guard data.count >= 5 else { return }
        
        let sampleRate = data[0..<4].withUnsafeBytes { $0.load(as: UInt32.self).bigEndian }
        let channels = data[4]
        let opusData = data[5...]
        
        // Decode Opus data
        guard let pcmData = opusDecoder.decode(Data(opusData)) else {
            print("Failed to decode Opus data")
            return
        }
        
        // Create audio buffer
        guard let buffer = createAudioBuffer(from: pcmData) else {
            print("Failed to create audio buffer")
            return
        }
        
        // Schedule buffer for playback
        playerNode.scheduleBuffer(buffer)
    }
    
    private func createAudioBuffer(from data: Data) -> AVAudioPCMBuffer? {
        let format = AVAudioFormat(
            commonFormat: .pcmFormatFloat32,
            sampleRate: sampleRate,
            channels: AVAudioChannelCount(channels),
            interleaved: false
        )!
        
        let frameCount = UInt32(data.count) / (UInt32(channels) * 4) // 4 bytes per float32 sample
        guard let buffer = AVAudioPCMBuffer(pcmFormat: format, frameCapacity: frameCount) else {
            return nil
        }
        
        buffer.frameLength = frameCount
        
        // Copy PCM data to buffer
        data.withUnsafeBytes { (ptr: UnsafeRawBufferPointer) in
            let floatPtr = ptr.bindMemory(to: Float.self)
            for channel in 0..<Int(channels) {
                if let channelData = buffer.floatChannelData?[channel] {
                    for frame in 0..<Int(frameCount) {
                        channelData[frame] = floatPtr[frame * Int(channels) + channel]
                    }
                }
            }
        }
        
        return buffer
    }
    
    func setVolume(_ volume: Float) {
        playerNode.volume = volume
    }
    
    func stop() {
        playerNode.stop()
        audioEngine.stop()
    }
}
