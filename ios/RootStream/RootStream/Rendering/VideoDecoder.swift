//
//  VideoDecoder.swift
//  RootStream iOS
//
//  Hardware video decoding using VideoToolbox (H.264/H.265/VP9)
//

import Foundation
import VideoToolbox
import CoreVideo

class VideoDecoder {
    private var decompressSession: VTDecompressionSession?
    private var formatDescription: CMFormatDescription?
    private var callback: ((CVPixelBuffer?) -> Void)?
    
    private var codecType: CMVideoCodecType = kCMVideoCodecType_H264
    
    init() {
        setupDecompressionSession()
    }
    
    deinit {
        cleanup()
    }
    
    private func setupDecompressionSession() {
        // Will be initialized when first frame arrives with codec info
    }
    
    func decode(_ data: Data, completion: @escaping (CVPixelBuffer?) -> Void) {
        self.callback = completion
        
        // Parse codec type and dimensions from data
        // Format: [codec_type: 1 byte][width: 2 bytes][height: 2 bytes][frame_data: rest]
        guard data.count >= 5 else {
            completion(nil)
            return
        }
        
        let codecByte = data[0]
        let width = data[1..<3].withUnsafeBytes { $0.load(as: UInt16.self).bigEndian }
        let height = data[3..<5].withUnsafeBytes { $0.load(as: UInt16.self).bigEndian }
        let frameData = data[5...]
        
        // Determine codec type
        codecType = codecTypeFromByte(codecByte)
        
        // Create or recreate decompression session if needed
        if decompressSession == nil || needsRecreateSession(width: Int(width), height: Int(height)) {
            createDecompressionSession(width: Int(width), height: Int(height))
        }
        
        // Create CMSampleBuffer
        guard let sampleBuffer = createSampleBuffer(from: frameData, width: Int(width), height: Int(height)) else {
            completion(nil)
            return
        }
        
        // Decode frame
        var flagsOut: VTDecodeInfoFlags = []
        let status = VTDecompressionSessionDecodeFrame(
            decompressSession!,
            sampleBuffer: sampleBuffer,
            flags: [._EnableAsynchronousDecompression],
            infoFlagsOut: &flagsOut,
            outputHandler: { [weak self] status, infoFlags, imageBuffer, presentationTimeStamp, presentationDuration in
                guard status == noErr, let imageBuffer = imageBuffer else {
                    self?.callback?(nil)
                    return
                }
                self?.callback?(imageBuffer)
            }
        )
        
        if status != noErr {
            print("Decode error: \(status)")
            completion(nil)
        }
    }
    
    private func createDecompressionSession(width: Int, height: Int) {
        cleanup()
        
        // Create format description
        var formatDesc: CMFormatDescription?
        let status = CMVideoFormatDescriptionCreate(
            allocator: kCFAllocatorDefault,
            codecType: codecType,
            width: Int32(width),
            height: Int32(height),
            extensions: nil,
            formatDescriptionOut: &formatDesc
        )
        
        guard status == noErr, let formatDesc = formatDesc else {
            print("Failed to create format description")
            return
        }
        
        self.formatDescription = formatDesc
        
        // Setup destination attributes
        let destinationAttributes: [CFString: Any] = [
            kCVPixelBufferPixelFormatTypeKey: kCVPixelFormatType_32BGRA,
            kCVPixelBufferWidthKey: width,
            kCVPixelBufferHeightKey: height,
            kCVPixelBufferMetalCompatibilityKey: true
        ]
        
        // Create decompression session
        var session: VTDecompressionSession?
        let sessionStatus = VTDecompressionSessionCreate(
            allocator: kCFAllocatorDefault,
            formatDescription: formatDesc,
            decoderSpecification: nil,
            imageBufferAttributes: destinationAttributes as CFDictionary,
            outputCallback: nil,
            decompressionSessionOut: &session
        )
        
        guard sessionStatus == noErr, let session = session else {
            print("Failed to create decompression session: \(sessionStatus)")
            return
        }
        
        self.decompressSession = session
    }
    
    private func createSampleBuffer(from data: Data, width: Int, height: Int) -> CMSampleBuffer? {
        guard let formatDesc = formatDescription else { return nil }
        
        var blockBuffer: CMBlockBuffer?
        let dataPointer = (data as NSData).bytes.assumingMemoryBound(to: UInt8.self)
        
        let status = CMBlockBufferCreateWithMemoryBlock(
            allocator: kCFAllocatorDefault,
            memoryBlock: nil,
            blockLength: data.count,
            blockAllocator: kCFAllocatorDefault,
            customBlockSource: nil,
            offsetToData: 0,
            dataLength: data.count,
            flags: 0,
            blockBufferOut: &blockBuffer
        )
        
        guard status == kCMBlockBufferNoErr, let blockBuffer = blockBuffer else {
            return nil
        }
        
        CMBlockBufferReplaceDataBytes(
            with: dataPointer,
            blockBuffer: blockBuffer,
            offsetIntoDestination: 0,
            dataLength: data.count
        )
        
        var sampleBuffer: CMSampleBuffer?
        let sampleStatus = CMSampleBufferCreate(
            allocator: kCFAllocatorDefault,
            dataBuffer: blockBuffer,
            dataReady: true,
            makeDataReadyCallback: nil,
            refcon: nil,
            formatDescription: formatDesc,
            sampleCount: 1,
            sampleTimingEntryCount: 0,
            sampleTimingArray: nil,
            sampleSizeEntryCount: 0,
            sampleSizeArray: nil,
            sampleBufferOut: &sampleBuffer
        )
        
        guard sampleStatus == noErr else { return nil }
        
        return sampleBuffer
    }
    
    private func codecTypeFromByte(_ byte: UInt8) -> CMVideoCodecType {
        switch byte {
        case 0x01: return kCMVideoCodecType_H264
        case 0x02: return kCMVideoCodecType_HEVC
        case 0x03: return kCMVideoCodecType_VP9
        default: return kCMVideoCodecType_H264
        }
    }
    
    private func needsRecreateSession(width: Int, height: Int) -> Bool {
        // Check if format changed
        return formatDescription == nil
    }
    
    private func cleanup() {
        if let session = decompressSession {
            VTDecompressionSessionInvalidate(session)
            decompressSession = nil
        }
        formatDescription = nil
    }
}
