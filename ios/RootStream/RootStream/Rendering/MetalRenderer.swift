//
//  MetalRenderer.swift
//  RootStream iOS
//
//  Metal-based video rendering with MTKViewDelegate
//

import Metal
import MetalKit
import CoreVideo

class MetalRenderer: NSObject {
    private var device: MTLDevice!
    private var commandQueue: MTLCommandQueue!
    private var pipelineState: MTLRenderPipelineState!
    private var textureCache: CVMetalTextureCache!
    private var currentTexture: MTLTexture?
    
    private var vertexBuffer: MTLBuffer!
    private var frameCount: Int = 0
    private var lastFrameTime: CFTimeInterval = 0
    
    override init() {
        super.init()
        setupMetal()
    }
    
    private func setupMetal() {
        // Get default Metal device
        guard let device = MTLCreateSystemDefaultDevice() else {
            fatalError("Metal is not supported on this device")
        }
        self.device = device
        
        // Create command queue
        guard let commandQueue = device.makeCommandQueue() else {
            fatalError("Failed to create command queue")
        }
        self.commandQueue = commandQueue
        
        // Create texture cache
        CVMetalTextureCacheCreate(kCFAllocatorDefault, nil, device, nil, &textureCache)
        
        // Setup render pipeline
        setupPipeline()
        
        // Create vertex buffer
        createVertexBuffer()
    }
    
    private func setupPipeline() {
        // Load shader library
        guard let library = device.makeDefaultLibrary() else {
            fatalError("Failed to create shader library")
        }
        
        let vertexFunction = library.makeFunction(name: "vertexShader")
        let fragmentFunction = library.makeFunction(name: "fragmentShader")
        
        // Create pipeline descriptor
        let pipelineDescriptor = MTLRenderPipelineDescriptor()
        pipelineDescriptor.vertexFunction = vertexFunction
        pipelineDescriptor.fragmentFunction = fragmentFunction
        pipelineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
        
        do {
            pipelineState = try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
        } catch {
            fatalError("Failed to create pipeline state: \(error)")
        }
    }
    
    private func createVertexBuffer() {
        // Full-screen quad vertices
        let vertices: [Float] = [
            -1.0, -1.0, 0.0, 1.0,  // bottom-left
             1.0, -1.0, 1.0, 1.0,  // bottom-right
            -1.0,  1.0, 0.0, 0.0,  // top-left
             1.0,  1.0, 1.0, 0.0   // top-right
        ]
        
        let vertexDataSize = vertices.count * MemoryLayout<Float>.stride
        vertexBuffer = device.makeBuffer(bytes: vertices, length: vertexDataSize, options: [])
    }
    
    func renderFrame(_ pixelBuffer: CVPixelBuffer) {
        // Create texture from pixel buffer
        var textureRef: CVMetalTexture?
        let width = CVPixelBufferGetWidth(pixelBuffer)
        let height = CVPixelBufferGetHeight(pixelBuffer)
        
        let status = CVMetalTextureCacheCreateTextureFromImage(
            kCFAllocatorDefault,
            textureCache,
            pixelBuffer,
            nil,
            .bgra8Unorm,
            width,
            height,
            0,
            &textureRef
        )
        
        guard status == kCVReturnSuccess,
              let textureRef = textureRef,
              let texture = CVMetalTextureGetTexture(textureRef) else {
            return
        }
        
        currentTexture = texture
        frameCount += 1
    }
    
    func getCurrentFPS() -> Double {
        let currentTime = CACurrentMediaTime()
        if lastFrameTime == 0 {
            lastFrameTime = currentTime
            return 0
        }
        
        let elapsed = currentTime - lastFrameTime
        if elapsed >= 1.0 {
            let fps = Double(frameCount) / elapsed
            frameCount = 0
            lastFrameTime = currentTime
            return fps
        }
        
        return 0
    }
}

extension MetalRenderer: MTKViewDelegate {
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        // Handle size changes if needed
    }
    
    func draw(in view: MTKView) {
        guard let drawable = view.currentDrawable,
              let texture = currentTexture else {
            return
        }
        
        // Create command buffer
        guard let commandBuffer = commandQueue.makeCommandBuffer() else { return }
        
        // Create render pass descriptor
        let renderPassDescriptor = MTLRenderPassDescriptor()
        renderPassDescriptor.colorAttachments[0].texture = drawable.texture
        renderPassDescriptor.colorAttachments[0].loadAction = .clear
        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 1)
        renderPassDescriptor.colorAttachments[0].storeAction = .store
        
        // Create render encoder
        guard let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor) else {
            return
        }
        
        renderEncoder.setRenderPipelineState(pipelineState)
        renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        renderEncoder.setFragmentTexture(texture, index: 0)
        renderEncoder.drawPrimitives(type: .triangleStrip, vertexStart: 0, vertexCount: 4)
        renderEncoder.endEncoding()
        
        commandBuffer.present(drawable)
        commandBuffer.commit()
    }
}
