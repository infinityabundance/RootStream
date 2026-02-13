//
//  Peer.swift
//  RootStream iOS
//
//  Model representing a discovered peer
//

import Foundation

struct Peer: Identifiable, Codable {
    let id: String
    let name: String
    let hostname: String
    let port: UInt16
    let isManual: Bool
    var lastSeen: Date?
    var publicKey: String?
    var deviceInfo: DeviceInfo?
    
    init(id: String, name: String, hostname: String, port: UInt16, isManual: Bool = false) {
        self.id = id
        self.name = name
        self.hostname = hostname
        self.port = port
        self.isManual = isManual
        self.lastSeen = Date()
    }
}

struct DeviceInfo: Codable {
    let osVersion: String
    let deviceModel: String
    let capabilities: [String]
}
