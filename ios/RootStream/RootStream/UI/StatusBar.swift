//
//  StatusBar.swift
//  RootStream iOS
//
//  HUD overlay showing connection status, FPS, and latency
//

import SwiftUI

struct StatusBar: View {
    let connectionStatus: String
    let fps: Double
    let latency: Int
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack(spacing: 8) {
                Circle()
                    .fill(statusColor)
                    .frame(width: 8, height: 8)
                
                Text(connectionStatus)
                    .font(.caption)
            }
            
            Text("FPS: \(String(format: "%.1f", fps))")
                .font(.caption)
            
            Text("Latency: \(latency)ms")
                .font(.caption)
        }
        .padding(8)
        .background(Color.black.opacity(0.7))
        .foregroundColor(.white)
        .cornerRadius(8)
    }
    
    private var statusColor: Color {
        if connectionStatus.contains("Connected") {
            return .green
        } else if connectionStatus.contains("Connecting") {
            return .yellow
        } else {
            return .red
        }
    }
}

#Preview {
    StatusBar(connectionStatus: "Connected", fps: 60.0, latency: 15)
        .preferredColorScheme(.dark)
}
