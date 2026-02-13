//
//  StreamView.swift
//  RootStream iOS
//
//  Main streaming view with video rendering and input controls
//

import SwiftUI
import MetalKit

struct StreamView: View {
    @EnvironmentObject var appState: AppState
    @StateObject private var streamingClient = StreamingClient()
    @StateObject private var inputController = InputController()
    @State private var showControls = true
    @State private var showHUD = true
    @State private var fps: Double = 0
    @State private var latency: Int = 0
    
    var body: some View {
        ZStack {
            // Metal rendering view
            MetalRenderView(renderer: streamingClient.renderer)
                .ignoresSafeArea()
            
            // On-screen controls
            if showControls {
                VStack {
                    Spacer()
                    
                    HStack {
                        // D-Pad
                        DPadView(inputController: inputController)
                            .frame(width: 120, height: 120)
                            .padding()
                        
                        Spacer()
                        
                        // Action buttons (A, B, X, Y)
                        ActionButtonsView(inputController: inputController)
                            .frame(width: 120, height: 120)
                            .padding()
                    }
                    
                    // Joystick (optional)
                    OnScreenJoystick(inputController: inputController)
                        .frame(width: 100, height: 100)
                        .padding()
                }
            }
            
            // HUD overlay
            if showHUD {
                VStack {
                    HStack {
                        StatusBar(
                            connectionStatus: appState.connectionStatus.description,
                            fps: fps,
                            latency: latency
                        )
                        .padding()
                        
                        Spacer()
                        
                        Button(action: { showHUD.toggle() }) {
                            Image(systemName: "eye.slash.fill")
                                .foregroundColor(.white)
                                .padding(8)
                                .background(Color.black.opacity(0.5))
                                .clipShape(Circle())
                        }
                        .padding()
                    }
                    
                    Spacer()
                }
            }
            
            // Connection overlay
            if !appState.isConnected {
                ConnectionOverlay(
                    status: appState.connectionStatus.description,
                    onConnect: connectToStream,
                    onDisconnect: disconnectFromStream
                )
            }
        }
        .navigationBarHidden(true)
        .onAppear {
            if appState.isConnected {
                startMetricsUpdates()
            }
        }
    }
    
    private func connectToStream() {
        guard let peer = appState.selectedPeer else { return }
        
        Task {
            do {
                try await streamingClient.connect(to: peer)
                appState.isConnected = true
                appState.connectionStatus = .connected
                startMetricsUpdates()
            } catch {
                appState.connectionStatus = .error(error.localizedDescription)
            }
        }
    }
    
    private func disconnectFromStream() {
        streamingClient.disconnect()
        appState.isConnected = false
        appState.connectionStatus = .disconnected
    }
    
    private func startMetricsUpdates() {
        Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { _ in
            fps = streamingClient.currentFPS
            latency = streamingClient.currentLatency
        }
    }
}

// MARK: - Supporting Views

struct ConnectionOverlay: View {
    let status: String
    let onConnect: () -> Void
    let onDisconnect: () -> Void
    
    var body: some View {
        VStack(spacing: 20) {
            Text(status)
                .font(.title2)
                .foregroundColor(.white)
            
            Button(action: onConnect) {
                Text("Connect")
                    .fontWeight(.semibold)
                    .padding()
                    .frame(maxWidth: 200)
                    .background(Color.blue)
                    .foregroundColor(.white)
                    .cornerRadius(10)
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color.black.opacity(0.8))
    }
}

struct MetalRenderView: UIViewRepresentable {
    let renderer: MetalRenderer?
    
    func makeUIView(context: Context) -> MTKView {
        let mtkView = MTKView()
        mtkView.device = MTLCreateSystemDefaultDevice()
        mtkView.delegate = renderer
        return mtkView
    }
    
    func updateUIView(_ uiView: MTKView, context: Context) {
        // Update view if needed
    }
}

#Preview {
    StreamView()
        .environmentObject(AppState.shared)
}
