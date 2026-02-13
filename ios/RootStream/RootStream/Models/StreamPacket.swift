//
//  StreamPacket.swift
//  RootStream iOS
//
//  Protocol packet definitions for streaming
//

import Foundation

enum PacketType: UInt8 {
    case videoFrame = 0x01
    case audioFrame = 0x02
    case inputEvent = 0x03
    case control = 0x04
    case keepAlive = 0x05
    case authentication = 0x06
}

struct StreamPacket {
    let type: PacketType
    let timestamp: UInt64
    let sequenceNumber: UInt32
    let data: Data
    
    var serialized: Data {
        var result = Data()
        result.append(type.rawValue)
        result.append(contentsOf: withUnsafeBytes(of: timestamp.bigEndian) { Array($0) })
        result.append(contentsOf: withUnsafeBytes(of: sequenceNumber.bigEndian) { Array($0) })
        result.append(data)
        return result
    }
    
    static func deserialize(_ data: Data) -> StreamPacket? {
        guard data.count >= 13 else { return nil }
        
        guard let type = PacketType(rawValue: data[0]) else { return nil }
        
        let timestamp = data[1..<9].withUnsafeBytes { $0.load(as: UInt64.self).bigEndian }
        let sequenceNumber = data[9..<13].withUnsafeBytes { $0.load(as: UInt32.self).bigEndian }
        let payload = data[13...]
        
        return StreamPacket(
            type: type,
            timestamp: timestamp,
            sequenceNumber: sequenceNumber,
            data: Data(payload)
        )
    }
}

struct VideoFrameData {
    let codecType: UInt8
    let width: UInt16
    let height: UInt16
    let frameData: Data
    
    var serialized: Data {
        var result = Data()
        result.append(codecType)
        result.append(contentsOf: withUnsafeBytes(of: width.bigEndian) { Array($0) })
        result.append(contentsOf: withUnsafeBytes(of: height.bigEndian) { Array($0) })
        result.append(frameData)
        return result
    }
}

struct AudioFrameData {
    let sampleRate: UInt32
    let channels: UInt8
    let audioData: Data
    
    var serialized: Data {
        var result = Data()
        result.append(contentsOf: withUnsafeBytes(of: sampleRate.bigEndian) { Array($0) })
        result.append(channels)
        result.append(audioData)
        return result
    }
}

struct InputEvent {
    enum InputType: UInt8 {
        case keyPress = 0x01
        case keyRelease = 0x02
        case mouseMove = 0x03
        case mouseButton = 0x04
        case gamepadButton = 0x05
        case gamepadAxis = 0x06
        case touch = 0x07
    }
    
    let inputType: InputType
    let timestamp: UInt64
    let data: Data
}
