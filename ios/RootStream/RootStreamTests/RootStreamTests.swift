//
//  RootStreamTests.swift
//  RootStreamTests
//
//  Unit tests for RootStream iOS
//

import XCTest
@testable import RootStream

final class RootStreamTests: XCTestCase {
    
    override func setUpWithError() throws {
        // Put setup code here
    }
    
    override func tearDownWithError() throws {
        // Put teardown code here
    }
    
    // MARK: - Keychain Tests
    func testKeychainStorage() throws {
        let keychain = KeychainManager()
        
        // Store credentials
        try keychain.store(username: "testuser", password: "testpass")
        
        // Load credentials
        let credentials = keychain.loadCredentials()
        XCTAssertNotNil(credentials)
        XCTAssertEqual(credentials?.username, "testuser")
        
        // Delete credentials
        try keychain.deleteCredentials()
    }
    
    // MARK: - Settings Tests
    func testUserDefaultsStorage() throws {
        let manager = UserDefaultsManager()
        
        var settings = UserDefaultsManager.Settings()
        settings.videoCodec = .h265
        settings.videoBitrate = 15_000_000
        settings.targetFPS = 30
        
        manager.saveSettings(settings)
        
        let loaded = manager.loadSettings()
        XCTAssertEqual(loaded.videoCodec, .h265)
        XCTAssertEqual(loaded.videoBitrate, 15_000_000)
        XCTAssertEqual(loaded.targetFPS, 30)
    }
    
    // MARK: - Packet Serialization Tests
    func testPacketSerialization() throws {
        let testData = "Hello, World!".data(using: .utf8)!
        
        let packet = StreamPacket(
            type: .videoFrame,
            timestamp: 1234567890,
            sequenceNumber: 42,
            data: testData
        )
        
        let serialized = packet.serialized
        let deserialized = StreamPacket.deserialize(serialized)
        
        XCTAssertNotNil(deserialized)
        XCTAssertEqual(deserialized?.type, .videoFrame)
        XCTAssertEqual(deserialized?.timestamp, 1234567890)
        XCTAssertEqual(deserialized?.sequenceNumber, 42)
        XCTAssertEqual(deserialized?.data, testData)
    }
    
    // MARK: - Encryption Tests
    func testEncryptionDecryption() throws {
        let securityManager = SecurityManager()
        
        // Authenticate to generate encryption key
        _ = try await securityManager.authenticate(username: "test", password: "test")
        
        let originalData = "Sensitive data".data(using: .utf8)!
        
        let encrypted = try securityManager.encrypt(originalData)
        XCTAssertNotEqual(encrypted, originalData)
        
        let decrypted = try securityManager.decrypt(encrypted)
        XCTAssertEqual(decrypted, originalData)
    }
    
    // MARK: - Performance Tests
    func testVideoDecoderPerformance() throws {
        let decoder = VideoDecoder()
        
        measure {
            // Simulate decoding 60 frames
            for _ in 0..<60 {
                let dummyData = Data(repeating: 0, count: 1024)
                decoder.decode(dummyData) { _ in }
            }
        }
    }
    
    func testMetalRendererPerformance() throws {
        let renderer = MetalRenderer()
        
        measure {
            // Simulate rendering 60 frames
            for _ in 0..<60 {
                let fps = renderer.getCurrentFPS()
                _ = fps
            }
        }
    }
}
